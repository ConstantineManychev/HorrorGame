#ifndef __GAME_DIRECTOR_H__
#define __GAME_DIRECTOR_H__

#include "CommonDefines.h"
#include "Basics/BaseLocation.h"
#include "Basics/Player.h"
#include "Basics/ServiceLocator.h" // Include ServiceLocator

_CSTART

class GameDirector
{
    friend class ServiceLocator;
    friend class AppDelegate;

private:
	BaseLocation* mCurrentLocation;
	Player* mCurrentPlayer;
    bool mIsEditorMode;

	GameDirector();

public:
	static GameDirector* getInstance();

	void startGame();
	void startEditor(); 
    
    bool isEditorMode() const { return mIsEditorMode; }

	void setLocation(BaseLocation* aLocation);
	void setPlayer(Player* aPlayer);

	BaseLocation* getCurrentLocation();
	Player* getCurrentPlayer();

};

#define GD SL->getService<GameDirector>()

_CEND

#endif // __GAME_DIRECTOR_H__
