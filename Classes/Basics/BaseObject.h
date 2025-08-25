#ifndef __BASE_OBJECT_H__
#define __BASE_OBJECT_H__

#include "cocos2d.h"

#include "CommonDefines.h"

_CSTART

class BaseObject : public Sprite
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

	Vec2 getSpeed();
	float getMaxSpeed();

	float getForceX();
	float getForceY();

	void setObjectPos(float aPosX, float aPosY);

protected:

	void update(float delta) override;

private:

	float mMaxSpeed;

	Vec2 mCurrentSpeed;
	Vec2 mCurrentForce;

	Vec2 mBodyPos;

	

};

_CEND

#endif // __BASE_OBJECT_H__
