#include "GameDirector.h"

#include "ScenesManager.h"
#include "Scenes/MainGameScene.h"
#include "DataManager.h"
#include "ViewManager.h"
#include "InputManager.h"
#include "Constants.h"

USING_NS_CC;
_CSTART

GameDirector::GameDirector()
	: mCurrentLocation(nullptr)
	, mCurrentPlayer(nullptr)
	, mIsEditorMode(false)
{

}

GameDirector* GameDirector::getInstance()
{
	static GameDirector instance;
	return &instance;
}

void GameDirector::startGame()
{
	mIsEditorMode = false;

	IM->switchContext(Constants::Contexts::GAME);

	DM->parseViewConfigs();

	auto mainGameScene = MainGameScene::create();
	SM->registerScene(Constants::Scenes::MAIN, mainGameScene);

	SM->openScene(Constants::Scenes::MAIN);
}

void GameDirector::startEditor()
{
	mIsEditorMode = true;

	IM->switchContext(Constants::Contexts::EDITOR);

	DM->parseViewConfigs();
	SM->runEditorScene();
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

_CEND