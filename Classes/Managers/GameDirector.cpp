#include "GameDirector.h"

#include "ScenesManager.h"
#include "Scenes/MainGameScene.h"
#include "DataManager.h"
#include "ViewManager.h"
#include "InputManager.h" // Added for context switching

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
    
    // Switch to Game Input Context
    IM->switchContext("Game");

	DM->parseViewConfigs();

	auto mainGameScene = MainGameScene::create();
	SM->registerScene("main", mainGameScene);

	SM->openScene("main");
}

void GameDirector::startEditor()
{
    mIsEditorMode = true;
    
    // Switch to Editor Input Context
    IM->switchContext("Editor");

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