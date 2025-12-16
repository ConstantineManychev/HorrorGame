#include "DataManager.h"
#include "Helpers/JsonHelper.h"
#include "Managers/GameDirector.h"

USING_NS_CC;
_CSTART


DataManager::DataManager() {}

void DataManager::setResourcePath(const std::string& aResPath) { mResourcePath = aResPath; }

void DataManager::setFullScreen(bool isFull) { mMainInfo.isFullScreen = isFull; }

void DataManager::updateScreenSize(const Size& newSize)
{
	mMainInfo.screenWidth = newSize.width;
	mMainInfo.screenHeight = newSize.height;
	calcScale();
}

void DataManager::setSpritesSize(const Size& newSize)
{
	mMainInfo.spritesWidth = newSize.width;
	mMainInfo.spritesHeight = newSize.height;
}

const sMainInfo& DataManager::getMainInfo() const { return mMainInfo; }

void DataManager::calcScale()
{
	auto screenSize = Director::getInstance()->getWinSize();
	float scaleX = screenSize.width / mMainInfo.spritesWidth;
	float scaleY = screenSize.height / mMainInfo.spritesHeight;
	setScale(scaleX, scaleY);
}

void DataManager::setScale(float aX, float aY)
{
	mMainInfo.scaleX = aX;
	mMainInfo.scaleY = aY;
}

float DataManager::getScaleY() const { return mMainInfo.scaleY; }
float DataManager::getScaleX() const { return mMainInfo.scaleX; }

void DataManager::addAtlasToLoad(const std::string& path) { mAtlasesToLoad.push_back(path); }

void DataManager::preloadResources()
{
	auto cache = SpriteFrameCache::getInstance();
	auto texCache = Director::getInstance()->getTextureCache();

	for (const auto& atlas : mAtlasesToLoad)
	{
		cache->addSpriteFramesWithFile(atlas);

		std::string texName = atlas;
		size_t pos = texName.rfind('.');
		if (pos != std::string::npos) texName.replace(pos, 4, ".png");

		auto tex = texCache->getTextureForKey(texName);
		if (tex) tex->setAliasTexParameters();
	}
	mAtlasesToLoad.clear();
}

void DataManager::registerViewInfo(const std::string& id, const ValueMap& info, const std::string& path)
{
	mViewsInfos[id] = info;
	mViewFilePaths[id] = path;
}

const ValueMap& DataManager::getViewInfoByID(const std::string& aID) const
{
	auto it = mViewsInfos.find(aID);
	if (it != mViewsInfos.end()) return it->second;
	static ValueMap empty;
	return empty;
}

std::string DataManager::getViewPath(const std::string& aID) const
{
	auto it = mViewFilePaths.find(aID);
	return (it != mViewFilePaths.end()) ? it->second : "";
}

std::vector<std::string> DataManager::getViewsIDs() const
{
	std::vector<std::string> viewIDs;
	for (const auto& pair : mViewsInfos) viewIDs.push_back(pair.first);
	return viewIDs;
}

void DataManager::setGlobalValues(const ValueMap& values) { mGlobalValues = values; }

const Value& DataManager::getGlobalValue(const std::string& aID)
{
	auto it = mGlobalValues.find(aID);
	return (it != mGlobalValues.end()) ? it->second : Value::Null;
}

void DataManager::setGlobalValue(const std::string& key, const Value& value)
{
	mGlobalValues[key] = value;
}

void DataManager::parseSceneObject(const ValueMap& aValue, sSceneObjectInfo& aSceneObjectInfo)
{
	aSceneObjectInfo.name = aValue.count("name") ? aValue.at("name").asString() : "";
	aSceneObjectInfo.type = aValue.count("type") ? aValue.at("type").asString() : "";
	if (aValue.count("position")) aSceneObjectInfo.position = JsonHelper::toVec2(aValue.at("position"));
	aSceneObjectInfo.scaleX = aValue.count("scaleX") ? aValue.at("scaleX").asFloat() : 1.0f;
	aSceneObjectInfo.scaleY = aValue.count("scaleY") ? aValue.at("scaleY").asFloat() : 1.0f;
	aSceneObjectInfo.zOrder = aValue.count("zOrder") ? aValue.at("zOrder").asInt() : 0;
	aSceneObjectInfo.textureFileName = aValue.count("textureFileName") ? aValue.at("textureFileName").asString() : "";
	if (aValue.count("customData")) aSceneObjectInfo.customData = aValue.at("customData");
}

void DataManager::loadScene(const std::string& sceneFilePath)
{
	mLoadedSceneObjects.clear();
	Value contentVal = JsonHelper::getValueFromJson(sceneFilePath);
	if (contentVal.getType() == Value::Type::VECTOR)
	{
		for (const auto& objVal : contentVal.asValueVector())
		{
			if (objVal.getType() == Value::Type::MAP)
			{
				sSceneObjectInfo objectInfo;
				parseSceneObject(objVal.asValueMap(), objectInfo);
				mLoadedSceneObjects.push_back(objectInfo);
			}
		}
	}
}

void DataManager::saveScene(const std::string& sceneFilePath, const std::vector<sSceneObjectInfo>& sceneObjects)
{
	ValueVector sceneArray;
	for (const auto& objInfo : sceneObjects)
	{
		ValueMap objMap;
		objMap["name"] = objInfo.name;
		objMap["type"] = objInfo.type;
		ValueMap posMap;
		posMap["x"] = objInfo.position.x;
		posMap["y"] = objInfo.position.y;
		objMap["position"] = posMap;
		objMap["scaleX"] = objInfo.scaleX;
		objMap["scaleY"] = objInfo.scaleY;
		objMap["zOrder"] = objInfo.zOrder;
		objMap["textureFileName"] = objInfo.textureFileName;
		if (!objInfo.customData.isNull()) objMap["customData"] = objInfo.customData;
		sceneArray.push_back(Value(objMap));
	}
	JsonHelper::saveValueToJson(sceneFilePath, Value(sceneArray));
}

void DataManager::saveView(const std::string& viewID, const std::vector<sSceneObjectInfo>& sceneObjects)
{
	ValueMap viewMap = getViewInfoByID(viewID);
	if (viewMap.empty()) viewMap["id"] = viewID;

	ValueMap childrenMap;
	for (const auto& objInfo : sceneObjects)
	{
		ValueMap objProperties;
		objProperties["type"] = objInfo.type;
		ValueMap paramsMap;
		paramsMap["pos_x"] = objInfo.position.x;
		paramsMap["pos_y"] = objInfo.position.y;
		paramsMap["scale_x"] = objInfo.scaleX;
		paramsMap["scale_y"] = objInfo.scaleY;
		paramsMap["layer"] = objInfo.zOrder;
		if (!objInfo.textureFileName.empty()) paramsMap["res"] = objInfo.textureFileName;
		objProperties["params"] = paramsMap;
		if (!objInfo.customData.isNull()) objProperties["customData"] = objInfo.customData;
		childrenMap[objInfo.name] = Value(objProperties);
	}
	viewMap["children"] = Value(childrenMap);
	mViewsInfos[viewID] = viewMap;

	std::string viewFilePath = getViewPath(viewID);
	if (viewFilePath.empty()) viewFilePath = "configs/views/" + viewID + ".json";
	JsonHelper::saveValueToJson(viewFilePath, Value(viewMap));
}

const std::vector<sSceneObjectInfo>& DataManager::getLoadedSceneObjects() const { return mLoadedSceneObjects; }

EventKeyboard::KeyCode DataManager::getKey(const std::string& aID)
{
	auto it = mKeys.find(aID);
	return (it != mKeys.end()) ? it->second : EventKeyboard::KeyCode::KEY_NONE;
}

_CEND