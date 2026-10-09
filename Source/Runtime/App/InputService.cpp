#include "Runtime/App/InputService.h"

#include <algorithm>
#include <array>
#include <utility>

namespace hg
{
    namespace
    {
        using KeyCode = ax::EventKeyboard::KeyCode;

        constexpr float kStickRadius = 90.0f;
        constexpr float kStickDeadZone = 0.2f;

        const std::unordered_map<std::string, KeyCode>& keyNames()
        {
            static const std::unordered_map<std::string, KeyCode> names = []
            {
                std::unordered_map<std::string, KeyCode> table{
                    {"SPACE", KeyCode::KEY_SPACE},
                    {"ESCAPE", KeyCode::KEY_ESCAPE},
                    {"ENTER", KeyCode::KEY_ENTER},
                    {"RETURN", KeyCode::KEY_ENTER},
                    {"TAB", KeyCode::KEY_TAB},
                    {"BACKSPACE", KeyCode::KEY_BACKSPACE},
                    {"SHIFT", KeyCode::KEY_SHIFT},
                    {"LEFT_SHIFT", KeyCode::KEY_LEFT_SHIFT},
                    {"RIGHT_SHIFT", KeyCode::KEY_RIGHT_SHIFT},
                    {"CTRL", KeyCode::KEY_CTRL},
                    {"LEFT_CTRL", KeyCode::KEY_LEFT_CTRL},
                    {"RIGHT_CTRL", KeyCode::KEY_RIGHT_CTRL},
                    {"ALT", KeyCode::KEY_ALT},
                    {"LEFT_ALT", KeyCode::KEY_LEFT_ALT},
                    {"RIGHT_ALT", KeyCode::KEY_RIGHT_ALT},
                    {"LEFT_ARROW", KeyCode::KEY_LEFT_ARROW},
                    {"RIGHT_ARROW", KeyCode::KEY_RIGHT_ARROW},
                    {"UP_ARROW", KeyCode::KEY_UP_ARROW},
                    {"DOWN_ARROW", KeyCode::KEY_DOWN_ARROW},
                    {"LEFT", KeyCode::KEY_LEFT_ARROW},
                    {"RIGHT", KeyCode::KEY_RIGHT_ARROW},
                    {"UP", KeyCode::KEY_UP_ARROW},
                    {"DOWN", KeyCode::KEY_DOWN_ARROW},
                    {"GRAVE", KeyCode::KEY_GRAVE},
                    {"MINUS", KeyCode::KEY_MINUS},
                    {"EQUAL", KeyCode::KEY_EQUAL},
                    {"COMMA", KeyCode::KEY_COMMA},
                    {"PERIOD", KeyCode::KEY_PERIOD},
                    {"SLASH", KeyCode::KEY_SLASH},
                    {"DELETE", KeyCode::KEY_DELETE},
                    {"HOME", KeyCode::KEY_HOME},
                    {"END", KeyCode::KEY_END},
                    {"BACK", KeyCode::KEY_BACK},
                };

                constexpr std::array<KeyCode, 26> letters{
                    KeyCode::KEY_A, KeyCode::KEY_B, KeyCode::KEY_C, KeyCode::KEY_D, KeyCode::KEY_E, KeyCode::KEY_F, KeyCode::KEY_G,
                    KeyCode::KEY_H, KeyCode::KEY_I, KeyCode::KEY_J, KeyCode::KEY_K, KeyCode::KEY_L, KeyCode::KEY_M, KeyCode::KEY_N,
                    KeyCode::KEY_O, KeyCode::KEY_P, KeyCode::KEY_Q, KeyCode::KEY_R, KeyCode::KEY_S, KeyCode::KEY_T, KeyCode::KEY_U,
                    KeyCode::KEY_V, KeyCode::KEY_W, KeyCode::KEY_X, KeyCode::KEY_Y, KeyCode::KEY_Z};
                for (size_t index = 0; index < letters.size(); ++index)
                {
                    table.emplace(std::string(1, static_cast<char>('A' + index)), letters[index]);
                }

                constexpr std::array<KeyCode, 10> digits{
                    KeyCode::KEY_0, KeyCode::KEY_1, KeyCode::KEY_2, KeyCode::KEY_3, KeyCode::KEY_4,
                    KeyCode::KEY_5, KeyCode::KEY_6, KeyCode::KEY_7, KeyCode::KEY_8, KeyCode::KEY_9};
                for (size_t index = 0; index < digits.size(); ++index)
                {
                    table.emplace(std::string(1, static_cast<char>('0' + index)), digits[index]);
                }

                constexpr std::array<KeyCode, 12> functions{
                    KeyCode::KEY_F1, KeyCode::KEY_F2, KeyCode::KEY_F3, KeyCode::KEY_F4, KeyCode::KEY_F5, KeyCode::KEY_F6,
                    KeyCode::KEY_F7, KeyCode::KEY_F8, KeyCode::KEY_F9, KeyCode::KEY_F10, KeyCode::KEY_F11, KeyCode::KEY_F12};
                for (size_t index = 0; index < functions.size(); ++index)
                {
                    table.emplace("F" + std::to_string(index + 1), functions[index]);
                }
                return table;
            }();
            return names;
        }

        bool touchControlsByDefault()
        {
#if (AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID) || (AX_TARGET_PLATFORM == AX_PLATFORM_IOS)
            return true;
#else
            return false;
#endif
        }
    }

    std::optional<ax::EventKeyboard::KeyCode> parseKeyName(std::string_view aName)
    {
        std::string name(aName);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char aCharacter)
        {
            return static_cast<char>(std::toupper(aCharacter));
        });
        if (name.rfind("KEY_", 0) == 0)
        {
            name.erase(0, 4);
        }
        const auto& names = keyNames();
        auto it = names.find(name);
        if (it == names.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    InputService::InputService()
        : mTouchControls(touchControlsByDefault())
    {
    }

    InputService::~InputService()
    {
        detach();
    }

    void InputService::configure(const std::vector<InputBinding>& aBindings)
    {
        mKeyActions.clear();
        for (const auto& binding : aBindings)
        {
            for (const auto& key : binding.keys)
            {
                if (auto code = parseKeyName(key))
                {
                    mKeyActions[*code].push_back(binding.action);
                }
                else
                {
                    AXLOGW("Unknown key '{}' in binding for '{}'", key, binding.action);
                }
            }
            state(binding.action);
        }
        reset();
    }

    void InputService::attach()
    {
        if (mKeyboardListener)
        {
            return;
        }
        auto* dispatcher = ax::Director::getInstance()->getEventDispatcher();

        mKeyboardListener = ax::EventListenerKeyboard::create();
        mKeyboardListener->onKeyPressed = [this](KeyCode aKey, ax::Event*)
        {
            onKey(aKey, true);
        };
        mKeyboardListener->onKeyReleased = [this](KeyCode aKey, ax::Event*)
        {
            onKey(aKey, false);
        };
        mKeyboardListener->retain();
        dispatcher->addEventListenerWithFixedPriority(mKeyboardListener, -1000);

        mTouchListener = ax::EventListenerTouchAllAtOnce::create();
        mTouchListener->onTouchesBegan = [this](const std::vector<ax::Touch*>& aTouches, ax::Event*)
        {
            onTouchesBegan(aTouches);
        };
        mTouchListener->onTouchesMoved = [this](const std::vector<ax::Touch*>& aTouches, ax::Event*)
        {
            onTouchesMoved(aTouches);
        };
        mTouchListener->onTouchesEnded = [this](const std::vector<ax::Touch*>& aTouches, ax::Event*)
        {
            onTouchesEnded(aTouches);
        };
        mTouchListener->onTouchesCancelled = mTouchListener->onTouchesEnded;
        mTouchListener->retain();
        dispatcher->addEventListenerWithFixedPriority(mTouchListener, -1000);
    }

    void InputService::detach()
    {
        auto* director = ax::Director::getInstance();
        auto* dispatcher = director ? director->getEventDispatcher() : nullptr;
        if (mKeyboardListener)
        {
            if (dispatcher)
            {
                dispatcher->removeEventListener(mKeyboardListener);
            }
            mKeyboardListener->release();
            mKeyboardListener = nullptr;
        }
        if (mTouchListener)
        {
            if (dispatcher)
            {
                dispatcher->removeEventListener(mTouchListener);
            }
            mTouchListener->release();
            mTouchListener = nullptr;
        }
    }

    void InputService::setEnabled(bool aEnabled)
    {
        mEnabled = aEnabled;
        if (!aEnabled)
        {
            reset();
        }
    }

    bool InputService::isEnabled() const
    {
        return mEnabled;
    }

    void InputService::setTouchControlsEnabled(bool aEnabled)
    {
        mTouchControls = aEnabled;
    }

    void InputService::reset()
    {
        mHeldKeys.clear();
        for (auto& [name, action] : mActions)
        {
            action.heldKeys = 0;
            action.heldByTouch = false;
            action.consumedCount = action.pressCount;
        }
        mStickTouchId = -1;
        mJumpTouchId = -1;
    }

    bool InputService::isHeld(std::string_view aAction) const
    {
        const ActionState* action = findState(aAction);
        return mEnabled && action && (action->heldKeys > 0 || action->heldByTouch);
    }

    bool InputService::consumePressed(std::string_view aAction)
    {
        ActionState& action = state(aAction);
        const bool pressed = action.pressCount != action.consumedCount;
        action.consumedCount = action.pressCount;
        return mEnabled && pressed;
    }

    Vec2 InputService::moveAxis() const
    {
        if (!mEnabled)
        {
            return {};
        }
        Vec2 axis;
        axis.x = (isHeld(InputActions::kMoveRight) ? 1.0f : 0.0f) - (isHeld(InputActions::kMoveLeft) ? 1.0f : 0.0f);
        axis.y = (isHeld(InputActions::kMoveUp) ? 1.0f : 0.0f) - (isHeld(InputActions::kMoveDown) ? 1.0f : 0.0f);

        if (mStickTouchId >= 0)
        {
            const ax::Vec2 delta = mStickCurrent - mStickOrigin;
            Vec2 stick{delta.x / kStickRadius, delta.y / kStickRadius};
            if (stick.length() > 1.0f)
            {
                stick = stick.normalized();
            }
            if (stick.length() > kStickDeadZone)
            {
                axis += stick;
            }
        }

        if (axis.length() > 1.0f)
        {
            axis = axis.normalized();
        }
        return axis;
    }

    void InputService::onKey(KeyCode aKey, bool aPressed)
    {
        auto bound = mKeyActions.find(aKey);
        if (bound == mKeyActions.end())
        {
            return;
        }
        if (aPressed)
        {
            if (!mHeldKeys.insert(aKey).second)
            {
                return;
            }
        }
        else if (mHeldKeys.erase(aKey) == 0)
        {
            return;
        }

        for (const auto& name : bound->second)
        {
            ActionState& action = state(name);
            if (aPressed)
            {
                ++action.heldKeys;
                ++action.pressCount;
            }
            else
            {
                action.heldKeys = std::max(0, action.heldKeys - 1);
            }
        }
    }

    void InputService::onTouchesBegan(const std::vector<ax::Touch*>& aTouches)
    {
        if (!mTouchControls)
        {
            return;
        }
        const float halfWidth = ax::Director::getInstance()->getVisibleOrigin().x + ax::Director::getInstance()->getVisibleSize().width * 0.5f;
        for (auto* touch : aTouches)
        {
            if (touch->getLocation().x < halfWidth && mStickTouchId < 0)
            {
                mStickTouchId = touch->getID();
                mStickOrigin = touch->getLocation();
                mStickCurrent = mStickOrigin;
            }
            else if (touch->getLocation().x >= halfWidth && mJumpTouchId < 0)
            {
                mJumpTouchId = touch->getID();
                setTouchAction(InputActions::kJump, true);
            }
        }
    }

    void InputService::onTouchesMoved(const std::vector<ax::Touch*>& aTouches)
    {
        for (auto* touch : aTouches)
        {
            if (touch->getID() == mStickTouchId)
            {
                mStickCurrent = touch->getLocation();
            }
        }
    }

    void InputService::onTouchesEnded(const std::vector<ax::Touch*>& aTouches)
    {
        for (auto* touch : aTouches)
        {
            if (touch->getID() == mStickTouchId)
            {
                mStickTouchId = -1;
            }
            if (touch->getID() == mJumpTouchId)
            {
                mJumpTouchId = -1;
                setTouchAction(InputActions::kJump, false);
            }
        }
    }

    void InputService::setTouchAction(std::string_view aAction, bool aHeld)
    {
        ActionState& action = state(aAction);
        if (aHeld && !action.heldByTouch)
        {
            ++action.pressCount;
        }
        action.heldByTouch = aHeld;
    }

    InputService::ActionState& InputService::state(std::string_view aAction)
    {
        auto it = mActions.find(std::string(aAction));
        if (it == mActions.end())
        {
            it = mActions.emplace(std::string(aAction), ActionState{}).first;
        }
        return it->second;
    }

    const InputService::ActionState* InputService::findState(std::string_view aAction) const
    {
        auto it = mActions.find(std::string(aAction));
        return it != mActions.end() ? &it->second : nullptr;
    }
}
