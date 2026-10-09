#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include "cocos2d.h"
#include "Basics/ServiceLocator.h"
#include "Types/InputTypes.h"
#include <map>

_CSTART

class InputManager
{
	friend class AppDelegate;
public:

	void createContext(const std::string& contextName);
	void bindKeyToContext(const std::string& contextName, cocos2d::EventKeyboard::KeyCode key, GameAction action);
	void setActiveContext(const std::string& contextName);

	void loadBindings(const cocos2d::ValueMap& bindingsMap);

	void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
	void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
	void onMouseMove(cocos2d::EventMouse* event);

	void setActionCallback(const std::function<void(GameAction, bool)>& callback);

private:
	InputManager();

	struct InputContext {
		std::string name;
		std::map<cocos2d::EventKeyboard::KeyCode, GameAction> keyBindings;
	};

	std::map<std::string, InputContext> mContexts;
	InputContext* mActiveContext;
	std::function<void(GameAction, bool)> mActionCallback;

	cocos2d::EventKeyboard::KeyCode convertStringToKeyCode(const std::string& keyName);
};

#define IM SL->getService<GameSpace::InputManager>()

_CEND

#endif