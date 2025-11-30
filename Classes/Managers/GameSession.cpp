#include "GameSession.h"
#include "Helpers/JsonHelper.h"

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
		Value configVal = JsonHelper::getValueFromJson("configs/new_game_config.json");

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

	void GameSession::saveSession(const std::string& filePath)
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

		JsonHelper::saveValueToJson(filePath, Value(saveMap));
	}

	void GameSession::loadSession(const std::string& filePath)
	{
		Value configVal = JsonHelper::getValueFromJson(filePath);

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

	void GameSession::setPlayerPosition(const Vec2& pos) { mPlayerPosition = pos; }
	Vec2 GameSession::getPlayerPosition() const { return mPlayerPosition; }

	void GameSession::setCurrentLocationID(const std::string& locationID) { mCurrentLocationID = locationID; }
	std::string GameSession::getCurrentLocationID() const { return mCurrentLocationID; }

	void GameSession::setGameTime(float time) { mGameTime = time; }
	float GameSession::getGameTime() const { return mGameTime; }
	void GameSession::addGameTime(float delta) { mGameTime += delta; }

	void GameSession::setSessionValue(const std::string& key, const Value& value) { mSessionData[key] = value; }

	Value GameSession::getSessionValue(const std::string& key) const
	{
		auto it = mSessionData.find(key);
		if (it != mSessionData.end()) return it->second;
		return Value::Null;
	}

}
