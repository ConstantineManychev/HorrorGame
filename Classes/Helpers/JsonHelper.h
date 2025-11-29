#ifndef __JsonHelper_H__
#define __JsonHelper_H__

#include "cocos2d.h"
#include "external/json/document.h"

namespace GameSpace {

	class JsonHelper
	{
	public:
		static cocos2d::ValueMap getValueMapFromJson(const std::string& aConfigPath);
		static cocos2d::Value getValueFromJson(const std::string& aConfigPath);

		static void saveValueToJson(const std::string& aConfigPath, const cocos2d::Value& aValue);

		static cocos2d::Vec2 toVec2(const cocos2d::Value& val);
		static cocos2d::Size toSize(const cocos2d::Value& val);
		static cocos2d::Color3B toColor3B(const cocos2d::Value& val);
		static cocos2d::Color4F toColor4F(const cocos2d::Value& val);

	private:
		static cocos2d::Value parseRapidJson(const rapidjson::Value& jsonValue);
		static void saveValueToRapidJson(rapidjson::Value& jsonVal, const cocos2d::Value& val, rapidjson::Document::AllocatorType& allocator);
	};

}

#endif