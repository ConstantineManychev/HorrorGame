#ifndef __ITEM_H__
#define __ITEM_H__

#include "BaseBeing.h"

#include "CommonDefines.h"

_CSTART

class Item : public BaseBeing
{
	BaseBeing* Parent;
public:

	static Item* create();

protected:

	void update(float delta) override;
};

_CEND

#endif // __ITEM_H__
