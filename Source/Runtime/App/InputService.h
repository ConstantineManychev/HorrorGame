#pragma once

#include "Core/App/AppConfig.h"
#include "Core/Base/Math.h"

#include "axmol.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hg
{
    namespace InputActions
    {
        inline constexpr std::string_view kMoveLeft = "move_left";
        inline constexpr std::string_view kMoveRight = "move_right";
        inline constexpr std::string_view kMoveUp = "move_up";
        inline constexpr std::string_view kMoveDown = "move_down";
        inline constexpr std::string_view kJump = "jump";
        inline constexpr std::string_view kUse = "use";
        inline constexpr std::string_view kRun = "run";
        inline constexpr std::string_view kPause = "pause";
    }

    std::optional<ax::EventKeyboard::KeyCode> parseKeyName(std::string_view aName);

    class InputService
    {
    public:
        InputService();
        ~InputService();

        InputService(const InputService&) = delete;
        InputService& operator=(const InputService&) = delete;

        void configure(const std::vector<InputBinding>& aBindings);
        void attach();
        void detach();

        void setEnabled(bool aEnabled);
        bool isEnabled() const;
        void setTouchControlsEnabled(bool aEnabled);
        void reset();

        bool isHeld(std::string_view aAction) const;
        bool consumePressed(std::string_view aAction);
        Vec2 moveAxis() const;

    private:
        struct ActionState
        {
            int heldKeys = 0;
            bool heldByTouch = false;
            unsigned pressCount = 0;
            unsigned consumedCount = 0;
        };

        void onKey(ax::EventKeyboard::KeyCode aKey, bool aPressed);
        void onTouchesBegan(const std::vector<ax::Touch*>& aTouches);
        void onTouchesMoved(const std::vector<ax::Touch*>& aTouches);
        void onTouchesEnded(const std::vector<ax::Touch*>& aTouches);
        void setTouchAction(std::string_view aAction, bool aHeld);
        ActionState& state(std::string_view aAction);
        const ActionState* findState(std::string_view aAction) const;

        std::unordered_map<ax::EventKeyboard::KeyCode, std::vector<std::string>> mKeyActions;
        std::unordered_map<std::string, ActionState> mActions;
        std::unordered_set<ax::EventKeyboard::KeyCode> mHeldKeys;
        ax::EventListenerKeyboard* mKeyboardListener = nullptr;
        ax::EventListenerTouchAllAtOnce* mTouchListener = nullptr;
        bool mEnabled = true;
        bool mTouchControls = false;
        int mStickTouchId = -1;
        int mJumpTouchId = -1;
        ax::Vec2 mStickOrigin;
        ax::Vec2 mStickCurrent;
    };
}
