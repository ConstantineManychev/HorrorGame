#ifndef __DATA_MANAGER_H__
#define __DATA_MANAGER_H__

#include "cocos2d.h"
#include "Types/BasicDataTypes.h"
#include "Basics/ServiceLocator.h"
#include "CommonDefines.h"
#include <map>
#include <string>
#include <vector>

_CSTART

class DataManager
{
	friend class AppDelegate;
public:

	// Runtime Data
	void setFullScreen(bool isFull);
	void updateScreenSize(const cocos2d::Size& newSize);
	void setSpritesSize(const cocos2d::Size& newSize);

	const sMainInfo& getMainInfo() const;

	void calcScale();
	float getScaleY() const;
	float getScaleX() const;
	void setScale(float aX, float aY);

	// Resources
	void setResourcePath(const std::string& aResPath);
	void addAtlasToLoad(const std::string& path);
	void preloadResources();

	// Views Data
	void registerViewInfo(const std::string& id, const cocos2d::ValueMap& info, const std::string& path);
	const cocos2d::ValueMap& getViewInfoByID(const std::string& aID) const;
	std::string getViewPath(const std::string& aID) const;
	std::vector<std::string> getViewsIDs() const;

	// Global Values
	void setGlobalValues(const cocos2d::ValueMap& values);
	const cocos2d::Value& getGlobalValue(const std::string& aID);
	void setGlobalValue(const std::string& key, const cocos2d::Value& value);

	// Scene Objects (Runtime / Editor)
	void loadScene(const std::string& sceneFilePath);
	void saveScene(const std::string& sceneFilePath, const std::vector<sSceneObjectInfo>& sceneObjects);
	void saveView(const std::string& viewID, const std::vector<sSceneObjectInfo>& sceneObjects);
	const std::vector<sSceneObjectInfo>& getLoadedSceneObjects() const;

	// Legacy / Input Helper
	cocos2d::EventKeyboard::KeyCode getKey(const std::string& aID);

private:
	DataManager();

	std::string mResourcePath;
	sMainInfo mMainInfo;
	std::vector<std::string> mAtlasesToLoad;

	std::map<std::string, cocos2d::ValueMap> mViewsInfos;
	std::map<std::string, std::string> mViewFilePaths;

	cocos2d::ValueMap mGlobalValues;
	std::map<std::string, cocos2d::EventKeyboard::KeyCode> mKeys; // Legacy storage

	std::vector<sSceneObjectInfo> mLoadedSceneObjects;

	void parseSceneObject(const cocos2d::ValueMap& aValue, sSceneObjectInfo& aSceneObjectInfo);
};

#define DM SL->getService<GameSpace::DataManager>()

_CEND

#endif