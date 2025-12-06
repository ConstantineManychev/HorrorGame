#ifndef __PHYSICS_MOVEMENT_COMPONENT_H__
#define __PHYSICS_MOVEMENT_COMPONENT_H__

#include "GameComponent.h"

_CSTART

class PhysicsMovementComponent : public GameComponent
{
public:
	static PhysicsMovementComponent* create();

	virtual bool init() override;
	virtual void update(float delta) override;

	void setVelocity(const cocos2d::Vec2& velocity);
	void addVelocity(const cocos2d::Vec2& velocity);
	cocos2d::Vec2 getVelocity() const;

private:
	cocos2d::Vec2 mDesiredVelocity;
};

_CEND

#endif