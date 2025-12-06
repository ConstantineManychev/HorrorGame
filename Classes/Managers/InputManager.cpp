#include "InputManager.h"
#include "EventBus.h"
#include "Managers/EventData.h"
#include "Constants.h"

USING_NS_CC;
_CSTART

InputManager* InputManager::getInstance()
{
	static InputManager instance;
	return &instance;
}

InputManager::InputManager()
{
	createContext(Constants::Contexts::GAME);
	createContext(Constants::Contexts::EDITOR);

	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_A, GameAction::MoveLeft);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_LEFT_ARROW, GameAction::MoveLeft);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_D, GameAction::MoveRight);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_RIGHT_ARROW, GameAction::MoveRight);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_W, GameAction::MoveUp);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_UP_ARROW, GameAction::MoveUp);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_S, GameAction::MoveDown);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_DOWN_ARROW, GameAction::MoveDown);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_SPACE, GameAction::Jump);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_E, GameAction::Use);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_ESCAPE, GameAction::Pause);
	bindKeyToContext(Constants::Contexts::GAME, EventKeyboard::KeyCode::KEY_P, GameAction::Pause);

	switchContext(Constants::Contexts::GAME);
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

void InputManager::createContext(const std::string& aContextName)
{
	if (mContexts.find(aContextName) == mContexts.end())
	{
		InputContext context;
		context.name = aContextName;
		mContexts[aContextName] = context;
	}
}

void InputManager::switchContext(const std::string& aContextName)
{
	auto it = mContexts.find(aContextName);
	if (it != mContexts.end())
	{
		mActiveContext = &it->second;
	}
}

void InputManager::bindKeyToContext(const std::string& aContextName, EventKeyboard::KeyCode aKeyCode, GameAction aAction)
{
	auto it = mContexts.find(aContextName);
	if (it != mContexts.end())
	{
		it->second.keyBindings[aKeyCode] = aAction;
	}
}

void InputManager::rebindKey(const std::string& aContextName, EventKeyboard::KeyCode aNewKey, GameAction aAction)
{
	auto it = mContexts.find(aContextName);
	if (it != mContexts.end())
	{
		auto& bindings = it->second.keyBindings;
		for (auto bIt = bindings.begin(); bIt != bindings.end(); )
		{
			if (bIt->second == aAction)
			{
				bIt = bindings.erase(bIt);
			}
			else
			{
				++bIt;
			}
		}
		bindings[aNewKey] = aAction;
	}
}

bool InputManager::isControllerConnected() const
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	return cocos2d::Controller::getAllController().size() > 0;
#else
	return true;
#endif
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

	InputActionEventData data((int)aAction, aIsPressed);
	EB->publish(EventType::INPUT_ACTION_TRIGGERED, &data);

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

	InputAxisEventData data((int)aAction, aValue);
	EB->publish(EventType::INPUT_AXIS_TRIGGERED, &data);

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