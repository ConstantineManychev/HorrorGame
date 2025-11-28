#ifndef __DATA_MANAGER_H__
#define __DATA_MANAGER_H__

#include "cocos2d.h"

//json
#include "external/json/reader.h"
#include "external/json/document.h"
#include "external/json/writer.h"
#include "external/json/stringbuffer.h"
#include "external/json/prettywriter.h"

#include "CommonDefines.h"

#include "Helpers/JsonHelper.h"
#include "Types/BasicDataTypes.h"
#include "Basics/BValue.h"

#include <map>

_CSTART

class DataManager
{
private:

	DataManager();

	void parseStartupInfo(const rapidjson::Value& aValue, sMainInfo& aMainInfo);

	std::string mResourcePath;
	std::string mMainInfoConfigPath; // New member to store the path of main config file

 	sMainInfo mMainInfo;
	BValueMap mGlobalValues;
	std::map<std::string, EventKeyboard::KeyCode> mKeys;

	std::map< std::string, sWindowInfo > mWindowsInfos;
	std::map< std::string, BValue > mViewsInfos;

	std::vector<sSceneObjectInfo> mLoadedSceneObjects;

	void parseViewConfig(const std::string& aConfigPath);

	EventKeyboard::KeyCode convertStringToKeyCode(const std::string& aID);

public:

	void parseSceneObject(const BValueMap& aValue, sSceneObjectInfo& aSceneObjectInfo);

	static DataManager* getInstance();

	void setResourcePath(const std::string& resPath);

	void loadMainInfo(const std::string& configPath);
	void loadWindows(const std::string& folderPath);
	void loadScene(const std::string& sceneFilePath);

	void saveScene(const std::string& sceneFilePath, const std::vector<sSceneObjectInfo>& sceneObjects);
	void saveView(const std::string& viewID, const std::vector<sSceneObjectInfo>& sceneObjects);

	const std::vector<sSceneObjectInfo>& getLoadedSceneObjects() const;

	void parseViewConfigs();
	std::vector<std::string> getViewsIDs() const; // New method to get all loaded view IDs

	const sMainInfo& getMainInfo() const;
	const sWindowInfo* getWindowInfo(const std::string& aWndID) const;

	const BValue& getViewInfoByID(const std::string& aID) const;

	void saveMainInfo();

	void calcScale();

	void setScale(float aX, float aY);
	void setScaleY(float aY);
	void setScaleX(float aX);

	float getScaleY();

	const BValue& getGlobalValue(const std::string& aID);
	EventKeyboard::KeyCode getKey(const std::string& aID);


};

#define DM DataManager::getInstance()

_CEND

#endif // __DATA_MANAGER_H__
