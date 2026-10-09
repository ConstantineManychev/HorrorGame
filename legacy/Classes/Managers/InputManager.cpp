#include "InputManager.h"
#include "Constants.h"

USING_NS_CC;
_CSTART

InputManager::InputManager() : mActiveContext(nullptr), mActionCallback(nullptr) {}

void InputManager::createContext(const std::string& contextName)
{
	if (mContexts.find(contextName) == mContexts.end())
	{
		InputContext context;
		context.name = contextName;
		mContexts[contextName] = context;
	}
}

void InputManager::bindKeyToContext(const std::string& contextName, EventKeyboard::KeyCode key, GameAction action)
{
	if (mContexts.find(contextName) != mContexts.end())
	{
		mContexts[contextName].keyBindings[key] = action;
	}
}

void InputManager::setActiveContext(const std::string& contextName)
{
	if (mContexts.find(contextName) != mContexts.end())
	{
		mActiveContext = &mContexts[contextName];
	}
}

void InputManager::setActionCallback(const std::function<void(GameAction, bool)>& callback)
{
	mActionCallback = callback;
}

void InputManager::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
	if (mActiveContext && mActionCallback)
	{
		auto it = mActiveContext->keyBindings.find(keyCode);
		if (it != mActiveContext->keyBindings.end())
		{
			mActionCallback(it->second, true);
		}
	}
}

void InputManager::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
	if (mActiveContext && mActionCallback)
	{
		auto it = mActiveContext->keyBindings.find(keyCode);
		if (it != mActiveContext->keyBindings.end())
		{
			mActionCallback(it->second, false);
		}
	}
}

void InputManager::onMouseMove(EventMouse* event)
{
}

void InputManager::loadBindings(const ValueMap& bindingsMap)
{
	createContext(Constants::Contexts::GAME);

	std::map<std::string, GameAction> strToAction = {
		{"MoveLeft", GameAction::MoveLeft},
		{"MoveRight", GameAction::MoveRight},
		{"MoveUp", GameAction::MoveUp},
		{"MoveDown", GameAction::MoveDown},
		{"Use", GameAction::Use},
		{"Jump", GameAction::Jump}
	};

	for (auto& pair : bindingsMap) {
		std::string actionStr = pair.first;
		if (strToAction.count(actionStr)) {
			GameAction action = strToAction[actionStr];
			if (pair.second.getType() == Value::Type::VECTOR) {
				for (const auto& keyVal : pair.second.asValueVector()) {
					bindKeyToContext(Constants::Contexts::GAME, convertStringToKeyCode(keyVal.asString()), action);
				}
			}
			else if (pair.second.getType() == Value::Type::STRING) {
				bindKeyToContext(Constants::Contexts::GAME, convertStringToKeyCode(pair.second.asString()), action);
			}
		}
	}
}

EventKeyboard::KeyCode InputManager::convertStringToKeyCode(const std::string& aID)
{
	if (aID == "KEY_A" || aID == "A") return EventKeyboard::KeyCode::KEY_A;
	if (aID == "KEY_D" || aID == "D") return EventKeyboard::KeyCode::KEY_D;
	if (aID == "KEY_S" || aID == "S") return EventKeyboard::KeyCode::KEY_S;
	if (aID == "KEY_W" || aID == "W") return EventKeyboard::KeyCode::KEY_W;
	if (aID == "KEY_E" || aID == "E") return EventKeyboard::KeyCode::KEY_E;
	if (aID == "KEY_Q" || aID == "Q") return EventKeyboard::KeyCode::KEY_Q;
	if (aID == "KEY_SPACE" || aID == "SPACE") return EventKeyboard::KeyCode::KEY_SPACE;
	if (aID == "KEY_SHIFT" || aID == "SHIFT") return EventKeyboard::KeyCode::KEY_SHIFT;
	if (aID == "KEY_CTRL" || aID == "CTRL") return EventKeyboard::KeyCode::KEY_CTRL;
	if (aID == "KEY_GRAVE" || aID == "`") return EventKeyboard::KeyCode::KEY_GRAVE;
	if (aID == "KEY_LEFT_ARROW" || aID == "LEFT") return EventKeyboard::KeyCode::KEY_LEFT_ARROW;
	if (aID == "KEY_RIGHT_ARROW" || aID == "RIGHT") return EventKeyboard::KeyCode::KEY_RIGHT_ARROW;
	if (aID == "KEY_UP_ARROW" || aID == "UP") return EventKeyboard::KeyCode::KEY_UP_ARROW;
	if (aID == "KEY_DOWN_ARROW" || aID == "DOWN") return EventKeyboard::KeyCode::KEY_DOWN_ARROW;
	return EventKeyboard::KeyCode::KEY_NONE;
}

_CEND