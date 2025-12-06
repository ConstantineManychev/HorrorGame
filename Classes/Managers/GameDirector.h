#ifndef __GAME_DIRECTOR_H__
#define __GAME_DIRECTOR_H__

#include "CommonDefines.h"
#include "Basics/BaseLocation.h"
#include "Basics/Player.h"
#include "Basics/ServiceLocator.h"

_CSTART

enum class GameState {
	LOGO,
	MAIN_MENU,
	GAMEPLAY,
	PAUSED
};

class GameDirector
{
	friend class ServiceLocator;
	friend class AppDelegate;

private:
	BaseLocation* mCurrentLocation;
	Player* mCurrentPlayer;
	bool mIsEditorMode;

	GameState mCurrentState;
	int mPauseMusicID;

	GameDirector();

	void startGameInternal();

public:
	static GameDirector* getInstance();

	void runApplicationFlow();
	void showMainMenu();

	void startNewGame();
	void loadGame(const std::string& aSaveSlot);
	void saveGame(const std::string& aSaveSlot);
	void exitGame();

	void pauseGame();
	void resumeGame();
	bool isPaused() const;

	void startGame();
	void startEditor();

	bool isEditorMode() const;

	void setLocation(BaseLocation* aLocation);
	void setPlayer(Player* aPlayer);

	BaseLocation* getCurrentLocation();
	Player* getCurrentPlayer();
};

#define GD SL->getService<GameDirector>()

_CEND

#endif