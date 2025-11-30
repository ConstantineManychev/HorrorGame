#include "DataManager.h"
#include "Helpers/JsonHelper.h"
#include "Managers/GameDirector.h"
#include "Managers/InputManager.h"
#include "Constants.h"

USING_NS_CC;

namespace GameSpace {

	DataManager::DataManager()
	{
	}

	DataManager* DataManager::getInstance()
	{
		static DataManager instance;
		return &instance;
	}

	void DataManager::setResourcePath(const std::string& aResPath)
	{
		mResourcePath = aResPath;
	}

	void DataManager::loadMainInfo(const std::string& aConfigPath)
	{
		Value configVal = JsonHelper::getValueFromJson(aConfigPath);

		if (configVal.getType() == Value::Type::MAP)
		{
			ValueMap configMap = configVal.asValueMap();

			if (configMap.find("startup_settings") != configMap.end())
			{
				Value settingsVal = configMap.at("startup_settings");
				if (settingsVal.getType() == Value::Type::MAP)
				{
					ValueMap settingsMap = settingsVal.asValueMap();

					if (settingsMap.count("is_full_screen"))
						mMainInfo.isFullScreen = settingsMap.at("is_full_screen").asBool();

					if (settingsMap.count("screen_size")) {
						Size sz = JsonHelper::toSize(settingsMap.at("screen_size"));
						mMainInfo.screenWidth = sz.width;
						mMainInfo.screenHeight = sz.height;
					}

					if (settingsMap.count("sprites_size")) {
						Size sz = JsonHelper::toSize(settingsMap.at("sprites_size"));
						mMainInfo.spritesWidth = sz.width;
						mMainInfo.spritesHeight = sz.height;
					}

					if (settingsMap.count("preload_atlases"))
					{
						const auto& atlases = settingsMap.at("preload_atlases").asValueVector();
						for (const auto& atlas : atlases)
						{
							SpriteFrameCache::getInstance()->addSpriteFramesWithFile(atlas.asString());
						}
					}

					if (settingsMap.count("input_bindings")) {
						ValueMap bindingsMap = settingsMap.at("input_bindings").asValueMap();

						std::map<std::string, GameAction> strToAction = {
							{"MoveLeft", GameAction::MoveLeft},
							{"MoveRight", GameAction::MoveRight},
							{"MoveUp", GameAction::MoveUp},
							{"MoveDown", GameAction::MoveDown},
							{"Use", GameAction::Use},
							{"Jump", GameAction::Jump},
							{"Run", GameAction::None},
							{"Console", GameAction::None}
						};

						for (auto& pair : bindingsMap) {
							std::string actionStr = pair.first;
							GameAction action = GameAction::None;
							if (strToAction.count(actionStr)) action = strToAction[actionStr];

							if (action != GameAction::None)
							{
								if (pair.second.getType() == Value::Type::VECTOR) {
									for (const auto& keyVal : pair.second.asValueVector()) {
										EventKeyboard::KeyCode code = convertStringToKeyCode(keyVal.asString());
										IM->bindKeyToContext(Constants::Contexts::GAME, code, action);
									}
								}
								else if (pair.second.getType() == Value::Type::STRING) {
									EventKeyboard::KeyCode code = convertStringToKeyCode(pair.second.asString());
									IM->bindKeyToContext(Constants::Contexts::GAME, code, action);
								}
							}
						}
					}
				}
			}

			if (configMap.find("global_values") != configMap.end())
			{
				Value globalVal = configMap.at("global_values");
				if (globalVal.getType() == Value::Type::MAP)
				{
					mGlobalValues = globalVal.asValueMap();
				}
			}
		}
	}

	void DataManager::parseViewConfigs()
	{
		Value values = JsonHelper::getValueFromJson("configs/views/views_list.json");

		if (values.getType() == Value::Type::VECTOR)
		{
			auto valVec = values.asValueVector();
			for (auto& val : valVec)
			{
				if (val.getType() == Value::Type::STRING)
				{
					parseViewConfig(val.asString());
				}
			}
		}
	}

	void DataManager::parseViewConfig(const std::string& aConfigPath)
	{
		Value val = JsonHelper::getValueFromJson(aConfigPath);

		if (val.getType() == Value::Type::MAP)
		{
			ValueMap valMap = val.asValueMap();

			if (valMap.find("id") != valMap.end())
			{
				std::string id = valMap.at("id").asString();
				mViewsInfos[id] = valMap;
				mViewFilePaths[id] = aConfigPath;
			}
		}
	}

	void DataManager::parseSceneObject(const ValueMap& aValue, sSceneObjectInfo& aSceneObjectInfo)
	{
		auto getString = [&](const std::string& key, const std::string& def) {
			return (aValue.find(key) != aValue.end()) ? aValue.at(key).asString() : def;
		};

		auto getFloat = [&](const std::string& key, float def) {
			return (aValue.find(key) != aValue.end()) ? aValue.at(key).asFloat() : def;
		};

		auto getInt = [&](const std::string& key, int def) {
			return (aValue.find(key) != aValue.end()) ? aValue.at(key).asInt() : def;
		};

		aSceneObjectInfo.name = getString("name", "");
		aSceneObjectInfo.type = getString("type", "");

		if (aValue.find("position") != aValue.end()) {
			aSceneObjectInfo.position = JsonHelper::toVec2(aValue.at("position"));
		}

		aSceneObjectInfo.scaleX = getFloat("scaleX", 1.0f);
		aSceneObjectInfo.scaleY = getFloat("scaleY", 1.0f);
		aSceneObjectInfo.zOrder = getInt("zOrder", 0);
		aSceneObjectInfo.textureFileName = getString("textureFileName", "");

		if (aValue.find("customData") != aValue.end())
		{
			aSceneObjectInfo.customData = aValue.at("customData");
		}
	}

	void DataManager::loadScene(const std::string& sceneFilePath)
	{
		mLoadedSceneObjects.clear();

		Value contentVal = JsonHelper::getValueFromJson(sceneFilePath);

		if (contentVal.isNull() || contentVal.getType() != Value::Type::VECTOR)
		{
			return;
		}

		ValueVector objects = contentVal.asValueVector();
		for (const auto& objVal : objects)
		{
			if (objVal.getType() == Value::Type::MAP)
			{
				sSceneObjectInfo objectInfo;
				parseSceneObject(objVal.asValueMap(), objectInfo);
				mLoadedSceneObjects.push_back(objectInfo);
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

			if (!objInfo.customData.isNull())
			{
				objMap["customData"] = objInfo.customData;
			}

			sceneArray.push_back(Value(objMap));
		}

		JsonHelper::saveValueToJson(sceneFilePath, Value(sceneArray));
	}

	void DataManager::saveView(const std::string& viewID, const std::vector<sSceneObjectInfo>& sceneObjects)
	{
		ValueMap viewMap;
		auto itExisting = mViewsInfos.find(viewID);
		if (itExisting != mViewsInfos.end())
		{
			viewMap = itExisting->second;
		}
		else
		{
			viewMap["id"] = viewID;
		}

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
			paramsMap["rotation"] = objInfo.rotation;
			paramsMap["layer"] = objInfo.zOrder;

			if (!objInfo.textureFileName.empty())
			{
				paramsMap["res"] = objInfo.textureFileName;
			}

			objProperties["params"] = paramsMap;

			if (!objInfo.customData.isNull())
			{
				objProperties["customData"] = objInfo.customData;
			}

			childrenMap[objInfo.name] = Value(objProperties);
		}

		viewMap["children"] = Value(childrenMap);

		mViewsInfos[viewID] = viewMap;

		std::string viewFilePath;
		auto itPath = mViewFilePaths.find(viewID);
		if (itPath != mViewFilePaths.end())
		{
			viewFilePath = itPath->second;
		}
		else
		{
			viewFilePath = "configs/views/" + viewID + ".json";
		}

		JsonHelper::saveValueToJson(viewFilePath, Value(viewMap));
	}

	const std::vector<sSceneObjectInfo>& DataManager::getLoadedSceneObjects() const
	{
		return mLoadedSceneObjects;
	}

	void DataManager::saveMainInfo()
	{
	}

	const sMainInfo& DataManager::getMainInfo() const
	{
		return mMainInfo;
	}

	float DataManager::getScaleY()
	{
		return mMainInfo.scaleY > mMainInfo.scaleX ? mMainInfo.scaleY : mMainInfo.scaleX;
	}

	void DataManager::setScale(float aX, float aY)
	{
		setScaleX(aX);
		setScaleY(aY);
	}

	void DataManager::setScaleY(float aY)
	{
		mMainInfo.scaleY = aY;
	}

	void DataManager::setScaleX(float aX)
	{
		mMainInfo.scaleX = aX;
	}

	void DataManager::calcScale()
	{
		auto screenSize = Director::getInstance()->getWinSize();
		setScale(screenSize.width / mMainInfo.spritesWidth, screenSize.height / mMainInfo.spritesHeight);
	}

	const ValueMap& DataManager::getViewInfoByID(const std::string& aID) const
	{
		auto it = mViewsInfos.find(aID);
		if (it != mViewsInfos.end())
		{
			return it->second;
		}
		static ValueMap empty;
		return empty;
	}

	std::vector<std::string> DataManager::getViewsIDs() const
	{
		std::vector<std::string> viewIDs;
		for (const auto& pair : mViewsInfos)
		{
			viewIDs.push_back(pair.first);
		}
		return viewIDs;
	}

	const Value& DataManager::getGlobalValue(const std::string& aID)
	{
		auto it = mGlobalValues.find(aID);
		if (it != mGlobalValues.end())
		{
			return it->second;
		}
		return Value::Null;
	}

	EventKeyboard::KeyCode DataManager::getKey(const std::string& aID)
	{
		EventKeyboard::KeyCode result = EventKeyboard::KeyCode::KEY_NONE;
		auto it = mKeys.find(aID);
		if (it != mKeys.end())
		{
			result = it->second;
		}
		return result;
	}

	EventKeyboard::KeyCode DataManager::convertStringToKeyCode(const std::string& aID)
	{
		if (aID == "KEY_A" || aID == "A") return EventKeyboard::KeyCode::KEY_A;
		if (aID == "KEY_D" || aID == "D") return EventKeyboard::KeyCode::KEY_D;
		if (aID == "KEY_S" || aID == "S") return EventKeyboard::KeyCode::KEY_S;
		if (aID == "KEY_W" || aID == "W") return EventKeyboard::KeyCode::KEY_W;
		if (aID == "KEY_E" || aID == "E") return EventKeyboard::KeyCode::KEY_E;
		if (aID == "KEY_Q" || aID == "Q") return EventKeyboard::KeyCode::KEY_Q;
		if (aID == "KEY_SPACE" || aID == "SPACE") return EventKeyboard::KeyCode::KEY_SPACE;
		if (aID == "KEY_SHIFT" || aID == "SHIFT") return EventKeyboard::KeyCode::KEY_SHIFT;
		if (aID == "KEY_CTRL" || aID == "CTRL") return EventKeyboard::KeyCode::KEY_CTRL;
		if (aID == "KEY_GRAVE" || aID == "`") return EventKeyboard::KeyCode::KEY_GRAVE;

		if (aID == "KEY_LEFT_ARROW" || aID == "LEFT") return EventKeyboard::KeyCode::KEY_LEFT_ARROW;
		if (aID == "KEY_RIGHT_ARROW" || aID == "RIGHT") return EventKeyboard::KeyCode::KEY_RIGHT_ARROW;
		if (aID == "KEY_UP_ARROW" || aID == "UP") return EventKeyboard::KeyCode::KEY_UP_ARROW;
		if (aID == "KEY_DOWN_ARROW" || aID == "DOWN") return EventKeyboard::KeyCode::KEY_DOWN_ARROW;

		return EventKeyboard::KeyCode::KEY_NONE;
	}

}