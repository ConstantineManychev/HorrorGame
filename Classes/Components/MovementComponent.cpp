#include "MovementComponent.h"

USING_NS_CC;
_CSTART

MovementComponent* MovementComponent::create()
{
    MovementComponent* pRet = new(std::nothrow) MovementComponent();
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

bool MovementComponent::init()
{
    if (!GameComponent::init()) return false;
    setName("MovementComponent");
    mVelocity = Vec2::ZERO;
    return true;
}

void MovementComponent::update(float delta)
{
    if (auto owner = getOwner())
    {
        if (!mVelocity.isZero())
        {
            Vec2 currentPos = owner->getPosition();
            owner->setPosition(currentPos + mVelocity * delta);
        }
    }
}

void MovementComponent::setVelocity(const Vec2& velocity)
{
    mVelocity = velocity;
}

void MovementComponent::addVelocity(const Vec2& velocity)
{
    mVelocity += velocity;
}

Vec2 MovementComponent::getVelocity() const
{
    return mVelocity;
}

_CEND
