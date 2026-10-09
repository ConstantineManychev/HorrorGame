#include "Item.h"

_USEC


Item* Item::create()
{
	Item* ret = new (std::nothrow) Item();
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

void Item::update(float delta)
{
    // Call parent update if necessary
    // BaseBeing::update(delta);
}