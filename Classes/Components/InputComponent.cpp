#include "InputComponent.h"

_USEC

InputComponent* InputComponent::create()
{
    InputComponent* pRet = new(std::nothrow) InputComponent();
    if (pRet && pRet->init())
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool InputComponent::init()
{
    if (!GameComponent::init()) return false;
    setName("InputComponent");
    
    // Generate unique listener ID based on instance address
    mListenerID = StringUtils::format("InputComp_%p", this);
    
    return true;
}

void InputComponent::onEnter()
{
    GameComponent::onEnter();
    
    // Register for all movement actions
    auto callback = [this](GameAction action, bool isPressed) {
        if (mCallback) mCallback(action, isPressed);
    };

    IM->addActionListener(GameAction::MoveLeft, mListenerID, callback);
    IM->addActionListener(GameAction::MoveRight, mListenerID, callback);
    IM->addActionListener(GameAction::MoveUp, mListenerID, callback);
    IM->addActionListener(GameAction::MoveDown, mListenerID, callback);
}

void InputComponent::onExit()
{
    IM->removeListener(mListenerID);
    GameComponent::onExit();
}

void InputComponent::setActionCallback(std::function<void(GameAction, bool)> callback)
{
    mCallback = callback;
}

