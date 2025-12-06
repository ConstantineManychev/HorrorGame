#include "GameDirector.h"
#include "ScenesManager.h"
#include "ViewManager.h"
#include "InputManager.h"
#include "DataManager.h"
#include "Constants.h"
#include "Managers/AudioManager.h"
#include "Managers/GameSession.h"

#include "Scenes/MainGameScene.h"

USING_NS_CC;
_CSTART

GameDirector::GameDirector()
	: mCurrentLocation(nullptr)
	, mCurrentPlayer(nullptr)
	, mIsEditorMode(false)
	, mCurrentState(GameState::LOGO)
	, mPauseMusicID(-1)
{
}

GameDirector* GameDirector::getInstance()
{
	static GameDirector instance;
	return &instance;
}

void GameDirector::runApplicationFlow()
{
	DM->parseViewConfigs();
	showMainMenu();
}

void GameDirector::showMainMenu()
{
	mCurrentState = GameState::MAIN_MENU;
	AM->playMusic(Constants::Sounds::MENU_THEME);

	auto titleScene = MainGameScene::create();

	SM->registerScene(Constants::Scenes::TITLE, titleScene);

	SM->openScene(Constants::Scenes::TITLE);
	VM->changeView(Constants::Views::TITLE);
}

void GameDirector::startNewGame()
{
	GS->startNewSession();
	startGameInternal();
}

void GameDirector::loadGame(const std::string& aSaveSlot)
{
	GS->loadSession(aSaveSlot);
	startGameInternal();
}

void GameDirector::saveGame(const std::string& aSaveSlot)
{
	if (mCurrentPlayer)
	{
		GS->setPlayerPosition(mCurrentPlayer->getPosition());
	}

	if (mCurrentLocation)
	{
		GS->setCurrentLocationID(mCurrentLocation->getName());
	}

	GS->saveSession(aSaveSlot);
}

void GameDirector::startGameInternal()
{
	mCurrentState = GameState::GAMEPLAY;

	AM->playMusic(Constants::Sounds::GAME_THEME);
	IM->switchContext(Constants::Contexts::GAME);

	SM->runGameScene();
	VM->changeView(Constants::Views::HUD);
}

void GameDirector::pauseGame()
{
	if (mCurrentState != GameState::GAMEPLAY) return;

	mCurrentState = GameState::PAUSED;

	if (SM->getCurrentScene())
	{
		SM->getCurrentScene()->getEventDispatcher()->setEnabled(false);
		auto children = SM->getCurrentScene()->getChildren();
		for (auto child : children)
		{
			child->pause();
		}
	}

	AM->pauseAllGameSounds();
	mPauseMusicID = AM->playUI(Constants::Sounds::PAUSE_THEME, true);

	VM->changeView(Constants::Views::PAUSE_MENU);
}

void GameDirector::resumeGame()
{
	if (mCurrentState != GameState::PAUSED) return;

	if (mPauseMusicID != -1)
	{
		AM->stopSound(mPauseMusicID);
		mPauseMusicID = -1;
	}

	AM->resumeAllGameSounds();

	VM->changeView(Constants::Views::HUD);

	if (SM->getCurrentScene())
	{
		SM->getCurrentScene()->getEventDispatcher()->setEnabled(true);
		auto children = SM->getCurrentScene()->getChildren();
		for (auto child : children)
		{
			child->resume();
		}
	}

	mCurrentState = GameState::GAMEPLAY;
}

void GameDirector::exitGame()
{
	Director::getInstance()->end();
}

void GameDirector::startGame()
{
	startNewGame();
}

void GameDirector::startEditor()
{
	mIsEditorMode = true;
	IM->switchContext(Constants::Contexts::EDITOR);
	DM->parseViewConfigs();
	SM->runEditorScene();
}

bool GameDirector::isPaused() const
{
	return mCurrentState == GameState::PAUSED;
}

bool GameDirector::isEditorMode() const
{
	return mIsEditorMode;
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