#ifndef __GAME_SESSION_H__
#define __GAME_SESSION_H__

#include "cocos2d.h"
#include "CommonDefines.h"

namespace GameSpace {

	class GameSession
	{
	public:
		static GameSession* getInstance();

		void startNewSession();
		void saveSession(const std::string& aFilePath);
		void loadSession(const std::string& aFilePath);

		void setPlayerPosition(const cocos2d::Vec2& aPos);
		cocos2d::Vec2 getPlayerPosition() const;

		void setCurrentLocationID(const std::string& aLocationID);
		std::string getCurrentLocationID() const;

		void setGameTime(float aTime);
		float getGameTime() const;
		void addGameTime(float aDelta);

		void setSessionValue(const std::string& aKey, const cocos2d::Value& aValue);
		cocos2d::Value getSessionValue(const std::string& aKey) const;

	private:
		GameSession();
		~GameSession();

		void loadDefaultConfig();

		cocos2d::Vec2 mPlayerPosition;
		std::string mCurrentLocationID;
		float mGameTime;

		cocos2d::ValueMap mSessionData;
	};

#define GS GameSpace::GameSession::getInstance()

}

#endif