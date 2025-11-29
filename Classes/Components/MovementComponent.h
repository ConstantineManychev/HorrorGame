#ifndef __MOVEMENT_COMPONENT_H__
#define __MOVEMENT_COMPONENT_H__

#include "GameComponent.h"

_CSTART

class MovementComponent : public GameComponent
{
public:
    static MovementComponent* create();
    
    virtual bool init() override;
    virtual void update(float delta) override;

    void setVelocity(const cocos2d::Vec2& velocity);
    void addVelocity(const cocos2d::Vec2& velocity);
	cocos2d::Vec2 getVelocity() const;

private:
	cocos2d::Vec2 mVelocity;
};

_CEND

#endif // __MOVEMENT_COMPONENT_H__

