#include "InputManager.h"

_USEC

InputManager* InputManager::getInstance()
{
    static InputManager instance;
    return &instance;
}

InputManager::InputManager()
{
    // Default bindings
    mKeyBindings[EventKeyboard::KeyCode::KEY_A] = GameAction::MoveLeft;
    mKeyBindings[EventKeyboard::KeyCode::KEY_LEFT_ARROW] = GameAction::MoveLeft;
    mKeyBindings[EventKeyboard::KeyCode::KEY_D] = GameAction::MoveRight;
    mKeyBindings[EventKeyboard::KeyCode::KEY_RIGHT_ARROW] = GameAction::MoveRight;
    mKeyBindings[EventKeyboard::KeyCode::KEY_W] = GameAction::MoveUp;
    mKeyBindings[EventKeyboard::KeyCode::KEY_UP_ARROW] = GameAction::MoveUp;
    mKeyBindings[EventKeyboard::KeyCode::KEY_S] = GameAction::MoveDown;
    mKeyBindings[EventKeyboard::KeyCode::KEY_DOWN_ARROW] = GameAction::MoveDown;
    mKeyBindings[EventKeyboard::KeyCode::KEY_SPACE] = GameAction::Jump;
    mKeyBindings[EventKeyboard::KeyCode::KEY_E] = GameAction::Use;
}

InputManager::~InputManager()
{
}

void InputManager::init(Node* aInputNode)
{
    if (!aInputNode) return;

    setupKeyboardListeners(aInputNode);
    setupMouseListeners(aInputNode);
}

void InputManager::setupKeyboardListeners(Node* aNode)
{
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = CC_CALLBACK_2(InputManager::onKeyPressed, this);
    listener->onKeyReleased = CC_CALLBACK_2(InputManager::onKeyReleased, this);
    
    aNode->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, aNode);
}

void InputManager::setupMouseListeners(Node* aNode)
{
    // Implement if needed for cursor
}

void InputManager::onKeyPressed(EventKeyboard::KeyCode aKeyCode, Event* aEvent)
{
    GameAction action = mapKeyToAction(aKeyCode);
    if (action != GameAction::None)
    {
        triggerAction(action, true);
        triggerAxis(action, 1.0f);
    }
}

void InputManager::onKeyReleased(EventKeyboard::KeyCode aKeyCode, Event* aEvent)
{
    GameAction action = mapKeyToAction(aKeyCode);
    if (action != GameAction::None)
    {
        triggerAction(action, false);
        triggerAxis(action, 0.0f);
    }
}

GameAction InputManager::mapKeyToAction(EventKeyboard::KeyCode aKeyCode)
{
    auto it = mKeyBindings.find(aKeyCode);
    if (it != mKeyBindings.end())
    {
        return it->second;
    }
    return GameAction::None;
}

void InputManager::triggerAction(GameAction aAction, bool aIsPressed)
{
    mActionStates[aAction] = aIsPressed;

    for (auto& pair : mActionListeners)
    {
        for (auto& info : pair.second)
        {
            if (info.action == aAction)
            {
                info.callback(aAction, aIsPressed);
            }
        }
    }
}

void InputManager::triggerAxis(GameAction aAction, float aValue)
{
    mAxisStates[aAction] = aValue;

    for (auto& pair : mAxisListeners)
    {
        for (auto& info : pair.second)
        {
            if (info.action == aAction)
            {
                info.callback(aAction, aValue);
            }
        }
    }
}

void InputManager::addActionListener(GameAction aAction, const std::string& aListenerID, ActionCallback aCallback)
{
    ActionListenerInfo info;
    info.action = aAction;
    info.callback = aCallback;
    mActionListeners[aListenerID].push_back(info);
}

void InputManager::addAxisListener(GameAction aAction, const std::string& aListenerID, AxisCallback aCallback)
{
    AxisListenerInfo info;
    info.action = aAction;
    info.callback = aCallback;
    mAxisListeners[aListenerID].push_back(info);
}

void InputManager::removeListener(const std::string& aListenerID)
{
    mActionListeners.erase(aListenerID);
    mAxisListeners.erase(aListenerID);
}

bool InputManager::isActionPressed(GameAction aAction) const
{
    auto it = mActionStates.find(aAction);
    if (it != mActionStates.end())
    {
        return it->second;
    }
    return false;
}

float InputManager::getActionStrength(GameAction aAction) const
{
    auto it = mAxisStates.find(aAction);
    if (it != mAxisStates.end())
    {
        return it->second;
    }
    return 0.0f;
}

void InputManager::bindKey(EventKeyboard::KeyCode aKeyCode, GameAction aAction)
{
    mKeyBindings[aKeyCode] = aAction;
}
