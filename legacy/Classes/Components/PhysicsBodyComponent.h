#ifndef __PHYSICS_BODY_COMPONENT_H__
#define __PHYSICS_BODY_COMPONENT_H__

#include "GameComponent.h"

_CSTART

class PhysicsBodyComponent : public GameComponent
{
public:
	static PhysicsBodyComponent* create(bool isDynamic = true);

	virtual bool init() override;
	virtual void onEnter() override;

	void setDynamic(bool isDynamic) { mIsDynamic = isDynamic; }
	void setCategoryBitmask(int mask) { mCategoryBitmask = mask; }
	void setCollisionBitmask(int mask) { mCollisionBitmask = mask; }
	void setContactTestBitmask(int mask) { mContactTestBitmask = mask; }

private:
	bool mIsDynamic;
	int mCategoryBitmask;
	int mCollisionBitmask;
	int mContactTestBitmask;
};

_CEND

#endif