#ifndef __GAME_DIRECTOR_H__
#define __GAME_DIRECTOR_H__

#include "cocos2d.h"
#include "Basics/ServiceLocator.h"
#include "CommonDefines.h"
#include "Basics/GameEntity.h"

_CSTART

class GameDirector
{
	friend class AppDelegate;
public:

	void runApplicationFlow();
	void startEditor();

	void showMainMenu();
	void startGame();

	bool isEditorMode() const { return mIsEditorMode; }
	GameEntity* getCurrentPlayer();

private:
	GameDirector();

	enum class GameState { NONE, INTRO, MAIN_MENU, GAMEPLAY, EDITOR };
	GameState mCurrentState;
	bool mIsEditorMode;
};

#define GD SL->getService<GameSpace::GameDirector>()

_CEND

#endif