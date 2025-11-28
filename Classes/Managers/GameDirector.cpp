#include "GameDirector.h"

#include "ScenesManager.h"

#include "Scenes/MainGameScene.h"
#include "DataManager.h"
#include "ViewManager.h"

_USEC

GameDirector::GameDirector()
	: mCurrentLocation(nullptr)
	, mCurrentPlayer(nullptr)
{

}

GameDirector* GameDirector::getInstance()
{
	static GameDirector instance;
	return &instance;
}

void GameDirector::startGame()
{
	DM->parseViewConfigs();

	auto mainGameScene = MainGameScene::create();
	SM->registerScene("main", mainGameScene);

	SM->openScene("main");
}

void GameDirector::setLocation(BaseLocation* aLocation)
{
	mCurrentLocation = aLocation;
}
void GameDirector::setPlayer(Player* aPlayer)
{
	mCurrentPlayer = aPlayer;
}


BaseLocation* GameDirector::getCurrentLocation()
{
	return mCurrentLocation;
}
Player* GameDirector::getCurrentPlayer()
{
	return mCurrentPlayer;
}