#include "PhysicsMovementComponent.h"

USING_NS_CC;
_CSTART

PhysicsMovementComponent* PhysicsMovementComponent::create()
{
	auto pRet = new(std::nothrow) PhysicsMovementComponent();
	if (pRet && pRet->init())
	{
		pRet->autorelease();
		return pRet;
	}
	delete pRet;
	return nullptr;
}

bool PhysicsMovementComponent::init()
{
	if (!GameComponent::init()) return false;
	setName("PhysicsMovementComponent");
	mDesiredVelocity = Vec2::ZERO;
	return true;
}

void PhysicsMovementComponent::setVelocity(const Vec2& velocity)
{
	mDesiredVelocity = velocity;
}

void PhysicsMovementComponent::addVelocity(const Vec2& velocity)
{
	mDesiredVelocity += velocity;
}

Vec2 PhysicsMovementComponent::getVelocity() const
{
	return mDesiredVelocity;
}

void PhysicsMovementComponent::update(float delta)
{
	auto owner = getOwner();
	if (!owner || !owner->getPhysicsBody()) return;

	owner->getPhysicsBody()->setVelocity(mDesiredVelocity);

	owner->getPhysicsBody()->setRotationEnable(false);
}

_CEND