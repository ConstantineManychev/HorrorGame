#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

#include "cocos2d.h"
#include "CommonDefines.h"

_CSTART

class GameEntity : public cocos2d::Node
{
public:
	static GameEntity* create();

	template <typename T>
	T* getGameComponent() { return dynamic_cast<T*>(this->getComponent(typeid(T).name())); }

	virtual bool init() override;
};

_CEND

#endif