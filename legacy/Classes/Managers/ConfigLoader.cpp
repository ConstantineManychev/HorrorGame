#include "ConfigLoader.h"
#include "Helpers/JsonHelper.h"
#include "Managers/DataManager.h"
#include "Managers/InputManager.h"
#include "Managers/ViewManager.h"
#include "Basics/ServiceLocator.h"
#include "Constants.h"

USING_NS_CC;
_CSTART

ConfigLoader::ConfigLoader() {}

void ConfigLoader::loadMainConfig(const std::string& configPath)
{
	Value configVal = JsonHelper::getValueFromJson(configPath);
	if (configVal.getType() != Value::Type::MAP) return;

	ValueMap configMap = configVal.asValueMap();

	// 1. Load Startup Settings
	if (configMap.count("startup_settings"))
	{
		ValueMap settingsMap = configMap.at("startup_settings").asValueMap();

		if (settingsMap.count("is_full_screen"))
			DM->setFullScreen(settingsMap.at("is_full_screen").asBool());

		if (settingsMap.count("screen_size"))
			DM->updateScreenSize(JsonHelper::toSize(settingsMap.at("screen_size")));

		if (settingsMap.count("sprites_size"))
			DM->setSpritesSize(JsonHelper::toSize(settingsMap.at("sprites_size")));

		if (settingsMap.count("preload_atlases"))
		{
			const auto& atlases = settingsMap.at("preload_atlases").asValueVector();
			for (const auto& atlas : atlases) DM->addAtlasToLoad(atlas.asString());
		}

		// Input Bindings
		if (settingsMap.count("input_bindings"))
		{
			ValueMap bindingsMap = settingsMap.at("input_bindings").asValueMap();
			IM->loadBindings(bindingsMap);
		}
	}

	// 2. Load Global Values
	if (configMap.count("global_values"))
	{
		DM->setGlobalValues(configMap.at("global_values").asValueMap());
	}
}

void ConfigLoader::loadViewConfigs(const std::string& listPath)
{
	Value values = JsonHelper::getValueFromJson(listPath);
	if (values.getType() == Value::Type::VECTOR)
	{
		auto valVec = values.asValueVector();
		for (auto& val : valVec)
		{
			parseViewConfig(val.asString());
		}
	}
}

void ConfigLoader::parseViewConfig(const std::string& configPath)
{
	Value val = JsonHelper::getValueFromJson(configPath);
	if (val.getType() == Value::Type::MAP)
	{
		ValueMap valMap = val.asValueMap();
		if (valMap.find("id") != valMap.end())
		{
			std::string id = valMap.at("id").asString();
			DM->registerViewInfo(id, valMap, configPath);
		}
	}
}

_CEND