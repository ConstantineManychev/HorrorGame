#ifndef __DATA_MANAGER_H__
#define __DATA_MANAGER_H__

#include "cocos2d.h"
#include "Types/BasicDataTypes.h"
#include "external/json/document.h" 
#include "Basics/ServiceLocator.h" 

namespace GameSpace {

	class DataManager
	{
		friend class ServiceLocator;
		friend class AppDelegate;

	public:
		static DataManager* getInstance();

		void loadMainInfo(const std::string& aConfigPath);
		void saveMainInfo();

		void parseViewConfigs();

		void loadScene(const std::string& sceneFilePath);
		void saveScene(const std::string& sceneFilePath, const std::vector<sSceneObjectInfo>& sceneObjects);
		void saveView(const std::string& viewID, const std::vector<sSceneObjectInfo>& sceneObjects);

		const std::vector<sSceneObjectInfo>& getLoadedSceneObjects() const;
		const sMainInfo& getMainInfo() const;

		void calcScale();
		float getScaleY();
		void setScale(float aX, float aY);
		void setScaleY(float aY);
		void setScaleX(float aX);

		const cocos2d::ValueMap& getViewInfoByID(const std::string& aID) const;
		std::vector<std::string> getViewsIDs() const;

		const cocos2d::Value& getGlobalValue(const std::string& aID);

		cocos2d::EventKeyboard::KeyCode getKey(const std::string& aID);

		void setResourcePath(const std::string& aResPath);

	private:
		DataManager();

		std::string mResourcePath;

		sMainInfo mMainInfo;

		std::map<std::string, cocos2d::ValueMap> mViewsInfos;
		std::map<std::string, std::string> mViewFilePaths;

		cocos2d::ValueMap mGlobalValues;
		std::map<std::string, cocos2d::EventKeyboard::KeyCode> mKeys;

		std::vector<sSceneObjectInfo> mLoadedSceneObjects;

		void parseViewConfig(const std::string& aConfigPath);
		void parseSceneObject(const cocos2d::ValueMap& aValue, sSceneObjectInfo& aSceneObjectInfo);

		cocos2d::EventKeyboard::KeyCode convertStringToKeyCode(const std::string& aID);
	};

}

#define DM SL->getService<GameSpace::DataManager>()

#endif