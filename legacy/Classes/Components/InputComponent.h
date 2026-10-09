#ifndef __INPUT_COMPONENT_H__
#define __INPUT_COMPONENT_H__

#include "GameComponent.h"
#include "Managers/InputManager.h" // Still needed for GameAction enum

_CSTART

class InputComponent : public GameComponent
{
public:
    static InputComponent* create();
    
    virtual bool init() override;
    virtual void onEnter() override;
    virtual void onExit() override;

    void setActionCallback(std::function<void(GameAction, bool)> callback);

private:
    size_t mBusListenerId = 0;
    std::function<void(GameAction, bool)> mCallback;
};

_CEND

#endif // __INPUT_COMPONENT_H__
