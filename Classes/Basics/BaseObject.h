#ifndef __BASE_OBJECT_H__
#define __BASE_OBJECT_H__

#include "cocos2d.h"

#include "CommonDefines.h"

_CSTART

class BaseObject : public cocos2d::Sprite
{
public:

	BaseObject();
	~BaseObject();

	static BaseObject* create();

	void setMaxSpeed(float aSpeed);

	void addForceX(float aForce);
	void addForceY(float aForce);

	void setForceX(float aForce);
	void setForceY(float aForce);

	cocos2d::Vec2 getSpeed();
	float getMaxSpeed();

	float getForceX();
	float getForceY();

	void setObjectPos(float aPosX, float aPosY);

protected:

	void update(float delta) override;

private:

	float mMaxSpeed;

	cocos2d::Vec2 mCurrentSpeed;
	cocos2d::Vec2 mCurrentForce;

	cocos2d::Vec2 mBodyPos;

	

};

_CEND

#endif // __BASE_OBJECT_H__
