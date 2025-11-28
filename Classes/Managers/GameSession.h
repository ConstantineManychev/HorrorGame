#ifndef __GAME_SESSION_H__
#define __GAME_SESSION_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Basics/BValue.h"

_CSTART

class GameSession
{
public:
	static GameSession* getInstance();

	void startNewSession();
	void saveSession(const std::string& filePath);
	void loadSession(const std::string& filePath);

	// Player Data
	void setPlayerPosition(const Vec2& pos);
	Vec2 getPlayerPosition() const;
	
	// You can extend this with more player stats like Health, Score, etc.
	// void setPlayerHealth(int health);
	// int getPlayerHealth() const;

	// Location Data
	void setCurrentLocationID(const std::string& locationID);
	std::string getCurrentLocationID() const;

	// Game Time
	void setGameTime(float time);
	float getGameTime() const;
	void addGameTime(float delta);

	// Generic Data
	void setSessionValue(const std::string& key, const BValue& value);
	BValue getSessionValue(const std::string& key) const;

private:
	GameSession();
	~GameSession();

	void loadDefaultConfig();

	Vec2 mPlayerPosition;
	std::string mCurrentLocationID;
	float mGameTime;
	
	BValueMap mSessionData;
};

#define GS GameSession::getInstance()

_CEND

#endif // __GAME_SESSION_H__

