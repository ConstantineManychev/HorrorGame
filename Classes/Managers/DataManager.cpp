#include "DataManager.h"

_USEC

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
	std::string content = FileUtils::getInstance()->getStringFromFile(aConfigPath);

	bool isLoadingCorrect = false;

	if (!content.empty())
	{
		rapidjson::Document document;
		document.Parse<0>(content.c_str());
		if (!document.HasParseError())
		{
			auto docIt = document.FindMember("startup_settings");
			if (docIt != document.MemberEnd())
			{
				isLoadingCorrect = true;
				parseStartupInfo(docIt->value, mMainInfo);
			}

			docIt = document.FindMember("global_values");
			if (docIt != document.MemberEnd() )
			{
				BValue data = BValue();
				JsonHelper::parseBValueFromJsonValue(docIt->value, data);

				if (data.isMap())
				{
					mGlobalValues = data.getValueMap();
				}
			}

		}
	}
}

void DataManager::parseStartupInfo(const rapidjson::Value& aValue, sMainInfo& aMainInfo)
{
	if (aValue.IsObject())
	{
		for (auto valIt = aValue.MemberBegin(); valIt != aValue.MemberEnd(); ++valIt)
		{
			if (valIt->name == "is_full_screen" && valIt->value.IsBool())
			{
				mMainInfo.isFullScreen = valIt->value.GetBool();
			}
			else if (valIt->name == "screen_size" && valIt->value.IsObject())
			{
				const auto& sizeObject = valIt->value.GetObject();

				auto heightIt = sizeObject.FindMember("height");
				auto widthIt = sizeObject.FindMember("width");

				if ( heightIt != sizeObject.MemberEnd()
					&& widthIt != sizeObject.MemberEnd())
				{
					if (heightIt->value.IsInt())
					{
						mMainInfo.screenHeight = static_cast<float>(heightIt->value.GetInt());
					}
					else if (heightIt->value.IsDouble() || heightIt->value.IsFloat())
					{
						mMainInfo.screenHeight = heightIt->value.GetFloat();
					}

					if (widthIt->value.IsInt())
					{
						mMainInfo.screenWidth = static_cast<float>(widthIt->value.GetInt());
					}
					else if (widthIt->value.IsDouble() || widthIt->value.IsFloat())
					{
						mMainInfo.screenWidth = widthIt->value.GetFloat();
					}
				}
			}
			else if (valIt->name == "sprites_size" && valIt->value.IsObject())
			{
				const auto& sizeObject = valIt->value.GetObject();

				auto heightIt = sizeObject.FindMember("height");
				auto widthIt = sizeObject.FindMember("width");

				if (heightIt != sizeObject.MemberEnd()
					&& widthIt != sizeObject.MemberEnd())
				{
					if (heightIt->value.IsInt())
					{
						mMainInfo.spritesHeight = static_cast<float>(heightIt->value.GetInt());
					}
					else if (heightIt->value.IsDouble() || heightIt->value.IsFloat())
					{
						mMainInfo.spritesHeight = heightIt->value.GetFloat();
					}

					if (widthIt->value.IsInt())
					{
						mMainInfo.spritesWidth = static_cast<float>(widthIt->value.GetInt());
					}
					else if (widthIt->value.IsDouble() || widthIt->value.IsFloat())
					{
						mMainInfo.spritesWidth = widthIt->value.GetFloat();
					}
				}
			}
			else if (valIt->name == "keys" && valIt->value.IsObject())
			{
				const auto& keysObject = valIt->value.GetObject();

				auto setupKey = [this, keysObject](const std::string& aID)
				{
					auto it = keysObject.FindMember(aID.c_str());
					if (it != keysObject.MemberEnd() && it->value.IsString())
					{
						mKeys[it->name.GetString()] = convertStringToKeyCode(it->value.GetString());
					}
				};

				setupKey("left");
				setupKey("right");
				setupKey("up");
				setupKey("down");
				
			}
		}
	}
}

void DataManager::parseViewConfigs()
{
	BValue values;
	JsonHelper::parseBValueFromJsonConfig("configs/views/views_list.json", values);

	if (values.isVector())
	{
		auto valVec = values.getValueVector();
		for (auto val : valVec)
		{
			if (val.isString())
			{
				parseViewConfig(val.getString());
			}
		}
	}
}

void DataManager::parseViewConfig(const std::string& aConfigPath)
{
	BValue values;
	JsonHelper::parseBValueFromJsonConfig(aConfigPath, values);
	if (values.isMap())
	{
		auto valMap = values.getValueMap();

		auto itID = valMap.find("id");
		if (itID != valMap.end() && itID->second.isString())
		{
			mViewsInfos[itID->second.getString()] = values;
		}
	}

}

void DataManager::parseSceneObject(const BValueMap& aValue, sSceneObjectInfo& aSceneObjectInfo)
{
	auto getFloatMember = [&](const std::string& name, float defaultValue) -> float {
		auto it = aValue.find(name);
		if (it != aValue.end() && (it->second.isFloat() || it->second.isDouble() || it->second.isInteger()))
		{
			return it->second.getFloat();
		}
		return defaultValue;
	};

	auto getIntMember = [&](const std::string& name, int defaultValue) -> int {
		auto it = aValue.find(name);
		if (it != aValue.end() && it->second.isInteger())
		{
			return it->second.getInt();
		}
		return defaultValue;
	};

	auto getStringMember = [&](const std::string& name, const std::string& defaultValue) -> std::string {
		auto it = aValue.find(name);
		if (it != aValue.end() && it->second.isString())
		{
			return it->second.getString();
		}
		return defaultValue;
	};

	auto itName = aValue.find("name");
	if (itName != aValue.end() && itName->second.isString())
	{
		aSceneObjectInfo.name = itName->second.getString();
	}

	auto itType = aValue.find("type");
	if (itType != aValue.end() && itType->second.isString())
	{
		aSceneObjectInfo.type = itType->second.getString();
	}

	auto itPosition = aValue.find("position");
	if (itPosition != aValue.end() && itPosition->second.isMap())
	{
		const auto& posObject = itPosition->second.getValueMap();
		aSceneObjectInfo.position.x = getFloatMember("x", 0.0f);
		aSceneObjectInfo.position.y = getFloatMember("y", 0.0f);
	}

	aSceneObjectInfo.scaleX = getFloatMember("scaleX", 1.0f);
	aSceneObjectInfo.scaleY = getFloatMember("scaleY", 1.0f);
	aSceneObjectInfo.zOrder = getIntMember("zOrder", 0);
	aSceneObjectInfo.textureFileName = getStringMember("textureFileName", "");

	auto itCustomData = aValue.find("customData");
	if (itCustomData != aValue.end())
	{
		aSceneObjectInfo.customData = itCustomData->second;
	}
}

void DataManager::loadScene(const std::string& sceneFilePath)
{
	mLoadedSceneObjects.clear();
	std::string content = FileUtils::getInstance()->getStringFromFile(sceneFilePath);

	if (content.empty())
	{
		CCLOG("DataManager: Failed to load scene file %s", sceneFilePath.c_str());
		return;
	}

	rapidjson::Document document;
	document.Parse<0>(content.c_str());

	if (document.HasParseError() || !document.IsArray())
	{
		CCLOG("DataManager: Failed to parse scene file %s or it's not an array.", sceneFilePath.c_str());
		return;
	}

	for (rapidjson::SizeType i = 0; i < document.Size(); ++i)
	{
		sSceneObjectInfo objectInfo;
		// Convert rapidjson::Value to BValueMap for parseSceneObject
		BValue objBValue;
		JsonHelper::parseBValueFromJsonValue(document[i], objBValue);
		if (objBValue.isMap())
		{
			parseSceneObject(objBValue.getValueMap(), objectInfo);
			mLoadedSceneObjects.push_back(objectInfo);
		}
	}
	CCLOG("DataManager: Scene %s loaded successfully with %zd objects.", sceneFilePath.c_str(), mLoadedSceneObjects.size());
}

void DataManager::saveScene(const std::string& sceneFilePath, const std::vector<sSceneObjectInfo>& sceneObjects)
{
	rapidjson::Document document;
	document.SetArray();
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	for (const auto& objInfo : sceneObjects)
	{
		rapidjson::Value obj(rapidjson::kObjectType);

		obj.AddMember("name", rapidjson::Value(objInfo.name.c_str(), allocator).Move(), allocator);
		obj.AddMember("type", rapidjson::Value(objInfo.type.c_str(), allocator).Move(), allocator);

		rapidjson::Value position(rapidjson::kObjectType);
		position.AddMember("x", objInfo.position.x, allocator);
		position.AddMember("y", objInfo.position.y, allocator);
		obj.AddMember("position", position, allocator);

		obj.AddMember("scaleX", objInfo.scaleX, allocator);
		obj.AddMember("scaleY", objInfo.scaleY, allocator);
		obj.AddMember("zOrder", objInfo.zOrder, allocator);
		obj.AddMember("textureFileName", rapidjson::Value(objInfo.textureFileName.c_str(), allocator).Move(), allocator);

		if (!objInfo.customData.isNull())
		{
			rapidjson::Value customDataValue;
			//JsonHelper::writeBValueToJsonValue(allocator, objInfo.customData, customDataValue);
			obj.AddMember("customData", customDataValue, allocator);
		}

		document.PushBack(obj, allocator);
	}

	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);

	std::string content = buffer.GetString();
	FileUtils::getInstance()->writeStringToFile(content, sceneFilePath);
	CCLOG("DataManager: Scene saved successfully to %s.", sceneFilePath.c_str());
}

void DataManager::saveView(const std::string& viewID, const std::vector<sSceneObjectInfo>& sceneObjects)
{
	CCLOG("DataManager: Saving view %s.", viewID.c_str());

	BValue viewBValue;
	viewBValue.clearWithType(BValue::Type::MAP);

	// Add view ID
	viewBValue.getValueMap()["id"] = BValue(viewID);

	// Create a BValue vector for children
	BValue childrenBValue;
	childrenBValue.clearWithType(BValue::Type::VECTOR);

	for (const auto& objInfo : sceneObjects)
	{
		BValue objMap;
		objMap.clearWithType(BValue::Type::MAP);

		objMap.getValueMap()["name"] = BValue(objInfo.name);
		objMap.getValueMap()["type"] = BValue(objInfo.type);

		BValue positionMap;
		positionMap.clearWithType(BValue::Type::MAP);
		positionMap.getValueMap()["x"] = BValue(objInfo.position.x);
		positionMap.getValueMap()["y"] = BValue(objInfo.position.y);
		objMap.getValueMap()["position"] = positionMap;

		objMap.getValueMap()["scaleX"] = BValue(objInfo.scaleX);
		objMap.getValueMap()["scaleY"] = BValue(objInfo.scaleY);
		objMap.getValueMap()["zOrder"] = BValue(objInfo.zOrder);
		objMap.getValueMap()["textureFileName"] = BValue(objInfo.textureFileName);

		if (!objInfo.customData.isNull())
		{
			objMap.getValueMap()["customData"] = objInfo.customData;
		}

		childrenBValue.getValueVector().push_back(objMap);
	}

	viewBValue.getValueMap()["children"] = childrenBValue;

	// Update mViewsInfos
	mViewsInfos[viewID] = viewBValue;

	// Get the actual file path for the view
	std::string viewFilePath = "configs/views/" + viewID + ".json"; // Assuming this convention
	// Alternatively, you might want to look up the original file path from parseViewConfig

	//JsonHelper::writeBValueToJsonConfig(viewFilePath, viewBValue);

	CCLOG("DataManager: View %s saved successfully to %s.", viewID.c_str(), viewFilePath.c_str());
}

const std::vector<sSceneObjectInfo>& DataManager::getLoadedSceneObjects() const
{
	return mLoadedSceneObjects;
}

void DataManager::saveMainInfo()
{
	// No implementation provided in the original file, so leaving it as is.
	CCLOG("DataManager::saveMainInfo() not implemented.");
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

const BValue& DataManager::getViewInfoByID(const std::string& aID) const
{
	auto it = mViewsInfos.find(aID);
	if (it != mViewsInfos.end())
	{
		return it->second;
	}

	return BValue::Null;
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

const BValue& DataManager::getGlobalValue(const std::string& aID)
{
	auto it = mGlobalValues.find(aID);
	if (it != mGlobalValues.end())
	{
		return it->second;
	}

	return BValue::Null;
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
	EventKeyboard::KeyCode result = EventKeyboard::KeyCode::KEY_NONE;
	
	if (aID == "A")
	{
		result = EventKeyboard::KeyCode::KEY_A;
	}
	else if (aID == "D")
	{
		result = EventKeyboard::KeyCode::KEY_D;
	}
	else if (aID == "S")
	{
		result = EventKeyboard::KeyCode::KEY_S;
	}
	else if (aID == "W")
	{
		result = EventKeyboard::KeyCode::KEY_W;
	}
	else if (aID == "E")
	{
		result = EventKeyboard::KeyCode::KEY_E;
	}
	else if (aID == "Q")
	{
		result = EventKeyboard::KeyCode::KEY_Q;
	}
	else if (aID == "I")
	{
		result = EventKeyboard::KeyCode::KEY_I;
	}
	else if (aID == "SPACE")
	{
		result = EventKeyboard::KeyCode::KEY_SPACE;
	}
	else if (aID == "SHIFT")
	{
		result = EventKeyboard::KeyCode::KEY_SHIFT;
	}
	else if (aID == "CTRL")
	{
		result = EventKeyboard::KeyCode::KEY_CTRL;
	}
	else if (aID == "ALT")
	{
		result = EventKeyboard::KeyCode::KEY_ALT;
	}
	else if (aID == "TAB")
	{
		result = EventKeyboard::KeyCode::KEY_TAB;
	}
	else if (aID == "ESC")
	{
		result = EventKeyboard::KeyCode::KEY_ESCAPE;
	}
	else if (aID == "`")
	{
		result = EventKeyboard::KeyCode::KEY_TILDE;
	}
	else if (aID == "CAPS LOCK")
	{
		result = EventKeyboard::KeyCode::KEY_CAPS_LOCK;
	}

	return result;
}