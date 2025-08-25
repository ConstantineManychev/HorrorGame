#include "BaseObject.h"
#include <algorithm> // For std::min

_USEC

BaseObject::BaseObject()
	: mMaxSpeed(1000.f)
	, mCurrentSpeed(Vec2::ZERO)
	, mCurrentForce(Vec2::ZERO)
	, mBodyPos(Vec2::ZERO)
{

}

BaseObject::~BaseObject()
{

}

BaseObject* BaseObject::create()
{
	BaseObject* ret = new (std::nothrow) BaseObject();
	if (ret && ret->init())
	{
		ret->autorelease();
		return ret;
	}
	else
	{
		delete ret;
		return nullptr;
	}
}

void BaseObject::setObjectPos(float aPosX, float aPosY)
{
	mBodyPos = Vec2(aPosX, aPosY);
	this->setPosition(aPosX, aPosY);
}

void BaseObject::update(float delta)
{
	Vec2 maxSpeed = Vec2::ZERO;

	bool isForceX = mCurrentForce.x != 0.f;
	bool isForceY = mCurrentForce.y != 0.f;

	if (isForceX)
	{
		maxSpeed.x = (mCurrentForce.x < 0.f ? (-1.f) : 1.f) * mMaxSpeed;
	}
	if (isForceY)
	{
		maxSpeed.y = (mCurrentForce.y < 0.f ? (-1.f) : 1.f) * mMaxSpeed;
	}

	if (mCurrentForce.isZero() && mCurrentSpeed.isZero())
	{
		this->stopAllActions();
		this->unscheduleUpdate();
	}
	else
	{
		Vec2 force = mCurrentForce;
		if (isForceX && isForceY)
		{
			force.x /= sqrt(2.f);
			force.y /= sqrt(2.f);
		}

		float timeX = delta;
		float timeY = delta;

		bool isMaxSpeedX = mCurrentSpeed.x == mMaxSpeed;
		bool isMaxSpeedY = mCurrentSpeed.y == mMaxSpeed;

		if (isForceX && !isMaxSpeedX)
		{
			float timeX = fabs(maxSpeed.x - mCurrentSpeed.x) / fabs(force.x);
		}

		if (isForceY && !isMaxSpeedY)
		{
			float timeY = fabs(maxSpeed.y - mCurrentSpeed.y) / fabs(force.y);
		}

		float time = std::min(timeX, std::min(timeY, delta));

		float distanceX = mCurrentSpeed.x * time + ( !isMaxSpeedX ? 0.5f * force.x * time * time : 0.f );
		float distanceY = mCurrentSpeed.y * time + ( !isMaxSpeedY ? 0.5f * force.y * time * time : 0.f );

		auto moveBy = MoveBy::create(time, Vec2(distanceX, distanceY));

		auto easeIn = EaseIn::create(moveBy, 2.f);

		if (!isMaxSpeedX)
		{
			mCurrentSpeed.x += !isForceX ? -mCurrentSpeed.x : force.x * time;
		}
		if (!isMaxSpeedY)
		{
			mCurrentSpeed.y += !isForceY ? -mCurrentSpeed.y : force.y * time;
		}

		this->runAction(easeIn);
	}
}

void BaseObject::setMaxSpeed(float aSpeed)
{
	mMaxSpeed = aSpeed;
}

void BaseObject::setForceX(float aForce)
{
	mCurrentForce.x = aForce;

	this->scheduleUpdate();
}

Vec2 BaseObject::getSpeed()
{
	return mCurrentSpeed;
}
float BaseObject::getMaxSpeed() 
{
	return mMaxSpeed;
}

float BaseObject::getForceX()
{
	return mCurrentForce.x;
}

void BaseObject::setForceY(float aForce)
{
	mCurrentForce.y = aForce;

	this->scheduleUpdate();
}

float BaseObject::getForceY()
{
	return mCurrentForce.y;
}

void BaseObject::addForceX(float aForce)
{
	mCurrentForce.x += aForce;

	this->scheduleUpdate();
}

void BaseObject::addForceY(float aForce)
{
	mCurrentForce.y += aForce;

	this->scheduleUpdate();
}