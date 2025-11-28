#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Types/InputTypes.h"
#include <functional>
#include <unordered_map>
#include <vector>

_CSTART

using ActionCallback = std::function<void(GameAction aAction, bool aIsPressed)>;
using AxisCallback = std::function<void(GameAction aAction, float aValue)>;

class InputManager
{
public:
    static InputManager* getInstance();

    void init(Node* aInputNode);

    void addActionListener(GameAction aAction, const std::string& aListenerID, ActionCallback aCallback);
    void addAxisListener(GameAction aAction, const std::string& aListenerID, AxisCallback aCallback);
    void removeListener(const std::string& aListenerID);

    bool isActionPressed(GameAction aAction) const;
    float getActionStrength(GameAction aAction) const;

    // Mapping configuration
    void bindKey(EventKeyboard::KeyCode aKeyCode, GameAction aAction);
    
private:
    InputManager();
    ~InputManager();

    void setupKeyboardListeners(Node* aNode);
    void setupMouseListeners(Node* aNode);
    // Controller support can be added later if needed to keep it simple first
    // void setupControllerListeners(Node* aNode);

    void onKeyPressed(EventKeyboard::KeyCode aKeyCode, Event* aEvent);
    void onKeyReleased(EventKeyboard::KeyCode aKeyCode, Event* aEvent);
    
    void triggerAction(GameAction aAction, bool aIsPressed);
    void triggerAxis(GameAction aAction, float aValue);

    GameAction mapKeyToAction(EventKeyboard::KeyCode aKeyCode);
    
    std::unordered_map<GameAction, bool> mActionStates;
    std::unordered_map<GameAction, float> mAxisStates;

    struct ActionListenerInfo {
        GameAction action;
        ActionCallback callback;
    };
    struct AxisListenerInfo {
        GameAction action;
        AxisCallback callback;
    };
    
    std::unordered_map<std::string, std::vector<ActionListenerInfo>> mActionListeners;
    std::unordered_map<std::string, std::vector<AxisListenerInfo>> mAxisListeners;

    std::unordered_map<EventKeyboard::KeyCode, GameAction> mKeyBindings;
};

#define IM InputManager::getInstance()

_CEND

#endif // __INPUT_MANAGER_H__
