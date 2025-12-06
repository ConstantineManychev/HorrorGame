#include "GameSession.h"
#include "Helpers/JsonHelper.h"
#include "Constants.h"

USING_NS_CC;

namespace GameSpace {

	GameSession::GameSession()
		: mPlayerPosition(Vec2::ZERO)
		, mCurrentLocationID("")
		, mGameTime(0.0f)
	{
	}

	GameSession::~GameSession()
	{
	}

	GameSession* GameSession::getInstance()
	{
		static GameSession instance;
		return &instance;
	}

	void GameSession::startNewSession()
	{
		loadDefaultConfig();
	}

	void GameSession::loadDefaultConfig()
	{
		Value configVal = JsonHelper::getValueFromJson(Constants::Configs::NEW_GAME);

		if (configVal.getType() == Value::Type::MAP)
		{
			const auto& map = configVal.asValueMap();

			if (map.count("player_position"))
			{
				mPlayerPosition = JsonHelper::toVec2(map.at("player_position"));
			}

			if (map.count("start_location_id"))
			{
				mCurrentLocationID = map.at("start_location_id").asString();
			}

			if (map.count("game_time"))
			{
				mGameTime = map.at("game_time").asFloat();
			}

			if (map.count("initial_data") && map.at("initial_data").getType() == Value::Type::MAP)
			{
				mSessionData = map.at("initial_data").asValueMap();
			}
		}
	}

	void GameSession::saveSession(const std::string& aFilePath)
	{
		ValueMap saveMap;

		ValueMap posMap;
		posMap["x"] = mPlayerPosition.x;
		posMap["y"] = mPlayerPosition.y;
		saveMap["player_position"] = posMap;

		saveMap["current_location_id"] = mCurrentLocationID;

		saveMap["game_time"] = mGameTime;

		if (!mSessionData.empty())
		{
			saveMap["session_data"] = mSessionData;
		}

		JsonHelper::saveValueToJson(aFilePath, Value(saveMap));
	}

	void GameSession::loadSession(const std::string& aFilePath)
	{
		Value configVal = JsonHelper::getValueFromJson(aFilePath);

		if (configVal.getType() == Value::Type::MAP)
		{
			const auto& map = configVal.asValueMap();

			if (map.count("player_position"))
			{
				mPlayerPosition = JsonHelper::toVec2(map.at("player_position"));
			}

			if (map.count("current_location_id"))
			{
				mCurrentLocationID = map.at("current_location_id").asString();
			}

			if (map.count("game_time"))
			{
				mGameTime = map.at("game_time").asFloat();
			}

			if (map.count("session_data") && map.at("session_data").getType() == Value::Type::MAP)
			{
				mSessionData = map.at("session_data").asValueMap();
			}
		}
	}

	void GameSession::setPlayerPosition(const Vec2& aPos) { mPlayerPosition = aPos; }
	Vec2 GameSession::getPlayerPosition() const { return mPlayerPosition; }

	void GameSession::setCurrentLocationID(const std::string& aLocationID) { mCurrentLocationID = aLocationID; }
	std::string GameSession::getCurrentLocationID() const { return mCurrentLocationID; }

	void GameSession::setGameTime(float aTime) { mGameTime = aTime; }
	float GameSession::getGameTime() const { return mGameTime; }
	void GameSession::addGameTime(float aDelta) { mGameTime += aDelta; }

	void GameSession::setSessionValue(const std::string& aKey, const Value& aValue) { mSessionData[aKey] = aValue; }

	Value GameSession::getSessionValue(const std::string& aKey) const
	{
		auto it = mSessionData.find(aKey);
		if (it != mSessionData.end()) return it->second;
		return Value::Null;
	}

}