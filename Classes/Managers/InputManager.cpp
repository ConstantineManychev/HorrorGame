#include "InputManager.h"
#include "EventBus.h"

USING_NS_CC;
_CSTART

InputManager* InputManager::getInstance()
{
    static InputManager instance;
    return &instance;
}

InputManager::InputManager()
{
    // Create default contexts
    createContext("Game");
    createContext("Editor");

    // Setup Game Context Bindings
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_A, GameAction::MoveLeft);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_LEFT_ARROW, GameAction::MoveLeft);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_D, GameAction::MoveRight);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_RIGHT_ARROW, GameAction::MoveRight);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_W, GameAction::MoveUp);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_UP_ARROW, GameAction::MoveUp);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_S, GameAction::MoveDown);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_DOWN_ARROW, GameAction::MoveDown);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_SPACE, GameAction::Jump);
    bindKeyToContext("Game", EventKeyboard::KeyCode::KEY_E, GameAction::Use);

    // Setup Editor Context Bindings (Initially empty or specific editor actions)
    // For example, WASD could move the camera in Editor, but NOT trigger "MoveUp" (Player movement)
    // bindKeyToContext("Editor", EventKeyboard::KeyCode::KEY_W, GameAction::EditorCameraUp); // Example
    
    // Set default context
    switchContext("Game");
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

void InputManager::createContext(const std::string& contextName)
{
    if (mContexts.find(contextName) == mContexts.end())
    {
        InputContext context;
        context.name = contextName;
        mContexts[contextName] = context;
    }
}

void InputManager::switchContext(const std::string& contextName)
{
    auto it = mContexts.find(contextName);
    if (it != mContexts.end())
    {
        // Reset current states if needed to prevent "stuck" keys
        // For simple implementation, we just switch pointer
        mActiveContext = &it->second;
        CCLOG("InputManager: Switched to context '%s'", contextName.c_str());
    }
    else
    {
        CCLOG("InputManager: Failed to switch to context '%s'. Not found.", contextName.c_str());
    }
}

void InputManager::bindKeyToContext(const std::string& contextName, EventKeyboard::KeyCode aKeyCode, GameAction aAction)
{
    auto it = mContexts.find(contextName);
    if (it != mContexts.end())
    {
        it->second.keyBindings[aKeyCode] = aAction;
    }
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
    if (mActiveContext)
    {
        auto it = mActiveContext->keyBindings.find(aKeyCode);
        if (it != mActiveContext->keyBindings.end())
        {
            return it->second;
        }
    }
    return GameAction::None;
}

void InputManager::triggerAction(GameAction aAction, bool aIsPressed)
{
    mActionStates[aAction] = aIsPressed;

    InputActionData data;
    data.action = (int)aAction;
    data.isPressed = aIsPressed;
    EB->publish(EventType::INPUT_ACTION_TRIGGERED, &data);

    // Legacy Listeners
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

    InputAxisData data;
    data.action = (int)aAction;
    data.value = aValue;
    EB->publish(EventType::INPUT_AXIS_TRIGGERED, &data);

    // Legacy Listeners
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

_CEND
