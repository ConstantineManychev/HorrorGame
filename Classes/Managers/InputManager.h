#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Types/InputTypes.h"
#include "Basics/ServiceLocator.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <map>

_CSTART

using ActionCallback = std::function<void(GameAction aAction, bool aIsPressed)>;
using AxisCallback = std::function<void(GameAction aAction, float aValue)>;

class InputManager
{
	friend class ServiceLocator;
	friend class AppDelegate;

public:
	static InputManager* getInstance();

	void init(cocos2d::Node* aInputNode);

	void createContext(const std::string& aContextName);
	void switchContext(const std::string& aContextName);
	void bindKeyToContext(const std::string& aContextName, cocos2d::EventKeyboard::KeyCode aKeyCode, GameAction aAction);
	void rebindKey(const std::string& aContextName, cocos2d::EventKeyboard::KeyCode aNewKey, GameAction aAction);

	void addActionListener(GameAction aAction, const std::string& aListenerID, ActionCallback aCallback);
	void addAxisListener(GameAction aAction, const std::string& aListenerID, AxisCallback aCallback);
	void removeListener(const std::string& aListenerID);

	bool isActionPressed(GameAction aAction) const;
	float getActionStrength(GameAction aAction) const;

	bool isControllerConnected() const;

private:
	InputManager();
	~InputManager();

	void setupKeyboardListeners(cocos2d::Node* aNode);
	void setupMouseListeners(cocos2d::Node* aNode);

	void onKeyPressed(cocos2d::EventKeyboard::KeyCode aKeyCode, cocos2d::Event* aEvent);
	void onKeyReleased(cocos2d::EventKeyboard::KeyCode aKeyCode, cocos2d::Event* aEvent);

	void triggerAction(GameAction aAction, bool aIsPressed);
	void triggerAxis(GameAction aAction, float aValue);

	GameAction mapKeyToAction(cocos2d::EventKeyboard::KeyCode aKeyCode);

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

	struct InputContext {
		std::string name;
		std::map<cocos2d::EventKeyboard::KeyCode, GameAction> keyBindings;
	};

	std::unordered_map<std::string, InputContext> mContexts;
	InputContext* mActiveContext = nullptr;
};

#define IM SL->getService<InputManager>()

_CEND

#endif