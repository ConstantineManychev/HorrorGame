#include "GameSession.h"
#include "Helpers/JsonHelper.h"

#include "cocos2d.h"

//json
#include "external/json/document.h"
#include "external/json/stringbuffer.h"
#include "external/json/prettywriter.h"

_USEC

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
    BValue configData;
    JsonHelper::parseBValueFromJsonConfig("configs/new_game_config.json", configData);
    
    if (configData.isMap())
    {
        const auto& map = configData.getValueMap();
        
        // Parse Player Position
        auto itPos = map.find("player_position");
        if (itPos != map.end() && itPos->second.isMap())
        {
            auto posMap = itPos->second.getValueMap();
            float x = 0.0f, y = 0.0f;
            if (posMap.count("x") && (posMap.at("x").isFloat() || posMap.at("x").isDouble())) 
                x = posMap.at("x").getFloat();
            if (posMap.count("y") && (posMap.at("y").isFloat() || posMap.at("y").isDouble())) 
                y = posMap.at("y").getFloat();
            mPlayerPosition = Vec2(x, y);
        }
        
        // Parse Start Location
        auto itLoc = map.find("start_location_id");
        if (itLoc != map.end() && itLoc->second.isString())
        {
            mCurrentLocationID = itLoc->second.getString();
        }
        
        // Parse Game Time
        auto itTime = map.find("game_time");
        if (itTime != map.end() && (itTime->second.isFloat() || itTime->second.isDouble()))
        {
            mGameTime = itTime->second.getFloat();
        }
        
        // Parse Initial Data
        auto itData = map.find("initial_data");
        if (itData != map.end() && itData->second.isMap())
        {
            mSessionData = itData->second.getValueMap();
        }
    }
}

void GameSession::saveSession(const std::string& filePath)
{
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    
    // Player Position
    rapidjson::Value posJson(rapidjson::kObjectType);
    posJson.AddMember("x", mPlayerPosition.x, allocator);
    posJson.AddMember("y", mPlayerPosition.y, allocator);
    document.AddMember("player_position", posJson, allocator);
    
    // Location
    rapidjson::Value locJson;
    locJson.SetString(mCurrentLocationID.c_str(), static_cast<rapidjson::SizeType>(mCurrentLocationID.length()), allocator);
    document.AddMember("current_location_id", locJson, allocator);
    
    // Game Time
    document.AddMember("game_time", mGameTime, allocator);
    
    // Session Data
    if (!mSessionData.empty())
    {
        BValue dataBVal(mSessionData);
        rapidjson::Value dataJson;
        JsonHelper::saveBValueToJsonValue(dataJson, dataBVal, allocator);
        document.AddMember("session_data", dataJson, allocator);
    }
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    
    std::string content = buffer.GetString();
    FileUtils::getInstance()->writeStringToFile(content, filePath);
}

void GameSession::loadSession(const std::string& filePath)
{
    BValue sessionData;
    JsonHelper::parseBValueFromJsonConfig(filePath, sessionData);
    
    if (sessionData.isMap())
    {
        const auto& map = sessionData.getValueMap();
        
        // Parse Player Position
        auto itPos = map.find("player_position");
        if (itPos != map.end() && itPos->second.isMap())
        {
            auto posMap = itPos->second.getValueMap();
            float x = 0.0f, y = 0.0f;
            if (posMap.count("x") && (posMap.at("x").isFloat() || posMap.at("x").isDouble())) 
                x = posMap.at("x").getFloat();
            if (posMap.count("y") && (posMap.at("y").isFloat() || posMap.at("y").isDouble())) 
                y = posMap.at("y").getFloat();
            mPlayerPosition = Vec2(x, y);
        }
        
        // Parse Start Location
        auto itLoc = map.find("current_location_id");
        if (itLoc != map.end() && itLoc->second.isString())
        {
            mCurrentLocationID = itLoc->second.getString();
        }
        
        // Parse Game Time
        auto itTime = map.find("game_time");
        if (itTime != map.end() && (itTime->second.isFloat() || itTime->second.isDouble()))
        {
            mGameTime = itTime->second.getFloat();
        }
        
        // Parse Session Data
        auto itData = map.find("session_data");
        if (itData != map.end() && itData->second.isMap())
        {
            mSessionData = itData->second.getValueMap();
        }
    }
}

// Accessors
void GameSession::setPlayerPosition(const Vec2& pos) { mPlayerPosition = pos; }
Vec2 GameSession::getPlayerPosition() const { return mPlayerPosition; }

void GameSession::setCurrentLocationID(const std::string& locationID) { mCurrentLocationID = locationID; }
std::string GameSession::getCurrentLocationID() const { return mCurrentLocationID; }

void GameSession::setGameTime(float time) { mGameTime = time; }
float GameSession::getGameTime() const { return mGameTime; }
void GameSession::addGameTime(float delta) { mGameTime += delta; }

void GameSession::setSessionValue(const std::string& key, const BValue& value) { mSessionData[key] = value; }
BValue GameSession::getSessionValue(const std::string& key) const
{
    auto it = mSessionData.find(key);
    if (it != mSessionData.end()) return it->second;
    return BValue();
}

