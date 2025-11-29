#include "InputComponent.h"
#include "Managers/EventBus.h"
#include "Managers/GameDirector.h"

USING_NS_CC;
_CSTART

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
    return true;
}

void InputComponent::onEnter()
{
    GameComponent::onEnter();
    
    // Subscribe to EventBus
    mBusListenerId = EB->subscribe(EventType::INPUT_ACTION_TRIGGERED, [this](void* data) {
        if (data)
        {
            InputActionData* inputData = static_cast<InputActionData*>(data);
            if (mCallback) 
            {
                mCallback((GameAction)inputData->action, inputData->isPressed);
            }
        }
    });
}

void InputComponent::onExit()
{
    if (mBusListenerId != 0)
    {
        EB->unsubscribe(EventType::INPUT_ACTION_TRIGGERED, mBusListenerId);
        mBusListenerId = 0;
    }
    GameComponent::onExit();
}

void InputComponent::setActionCallback(std::function<void(GameAction, bool)> callback)
{
    mCallback = callback;
}

_CEND