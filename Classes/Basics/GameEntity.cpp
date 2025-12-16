#include "GameEntity.h"

USING_NS_CC;
_CSTART

GameEntity* GameEntity::create()
{
	GameEntity* pRet = new(std::nothrow) GameEntity();
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

bool GameEntity::init()
{
	if (!Node::init()) return false;

	this->setCascadeOpacityEnabled(true);
	this->setCascadeColorEnabled(true);

	return true;
}

_CEND