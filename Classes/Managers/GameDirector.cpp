#include "GameDirector.h"
#include "ScenesManager.h"
#include "ViewManager.h"
#include "AudioManager.h"
#include "Constants.h"

USING_NS_CC;
_CSTART

GameDirector::GameDirector() : mCurrentState(GameState::NONE), mIsEditorMode(false) {}

void GameDirector::runApplicationFlow()
{
	mIsEditorMode = false;
	showMainMenu();
}

void GameDirector::startEditor()
{
	mIsEditorMode = true;
	mCurrentState = GameState::EDITOR;
	SM->openScene(Constants::Scenes::EDITOR);
}

void GameDirector::showMainMenu()
{
	mCurrentState = GameState::MAIN_MENU;
	AM->playMusic(Constants::Sounds::MENU_THEME);

	SM->openScene(Constants::Scenes::TITLE);
	VM->changeView(Constants::Views::TITLE);
}

void GameDirector::startGame()
{
	mCurrentState = GameState::GAMEPLAY;
	SM->openScene(Constants::Scenes::GAME);
	VM->changeView(Constants::Views::STAGE_A);
}

GameEntity* GameDirector::getCurrentPlayer()
{
	auto scene = dynamic_cast<Node*>(SM->getCurrentScene());
	if (scene) {
		return dynamic_cast<GameEntity*>(scene->getChildByName("Player"));
	}
	return nullptr;
}

_CEND