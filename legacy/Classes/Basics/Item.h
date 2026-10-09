#ifndef __ITEM_H__
#define __ITEM_H__

#include "GameEntity.h"

#include "CommonDefines.h"

_CSTART

class Item : public GameEntity
{
	GameEntity* Parent;
public:

	static Item* create();

protected:

	void update(float delta) override;
};

_CEND

#endif // __ITEM_H__
