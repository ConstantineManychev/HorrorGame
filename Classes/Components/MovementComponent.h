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

    void setVelocity(const Vec2& velocity);
    void addVelocity(const Vec2& velocity);
    Vec2 getVelocity() const;

private:
    Vec2 mVelocity;
};

_CEND

#endif // __MOVEMENT_COMPONENT_H__

