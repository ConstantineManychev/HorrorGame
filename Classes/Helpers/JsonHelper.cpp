#include "JsonHelper.h"
#include "external/json/stringbuffer.h"
#include "external/json/prettywriter.h"

USING_NS_CC;

namespace GameSpace {

	ValueMap JsonHelper::getValueMapFromJson(const std::string& aConfigPath)
	{
		Value val = getValueFromJson(aConfigPath);
		if (val.getType() == Value::Type::MAP) {
			return val.asValueMap();
		}
		return ValueMap();
	}

	Value JsonHelper::getValueFromJson(const std::string& aConfigPath)
	{
		std::string content = FileUtils::getInstance()->getStringFromFile(aConfigPath);
		if (content.empty()) return Value::Null;

		rapidjson::Document doc;
		doc.Parse<0>(content.c_str());
		if (doc.HasParseError()) {
			return Value::Null;
		}

		return parseRapidJson(doc);
	}

	void JsonHelper::saveValueToJson(const std::string& aConfigPath, const Value& aValue)
	{
		rapidjson::Document doc;

		if (aValue.getType() == Value::Type::VECTOR) {
			doc.SetArray();
		}
		else {
			doc.SetObject();
		}

		saveValueToRapidJson(doc, aValue, doc.GetAllocator());

		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		std::string fullPath = FileUtils::getInstance()->fullPathForFilename(aConfigPath);
		if (fullPath.empty()) fullPath = aConfigPath;

		FileUtils::getInstance()->writeStringToFile(buffer.GetString(), fullPath);
	}

	Value JsonHelper::parseRapidJson(const rapidjson::Value& v)
	{
		if (v.IsBool()) return Value(v.GetBool());
		if (v.IsInt()) return Value(v.GetInt());
		if (v.IsDouble()) return Value(v.GetDouble());
		if (v.IsString()) return Value(v.GetString());

		if (v.IsArray()) {
			ValueVector vec;
			for (auto& elem : v.GetArray()) {
				vec.push_back(parseRapidJson(elem));
			}
			return Value(vec);
		}

		if (v.IsObject()) {
			ValueMap map;
			for (auto& m : v.GetObject()) {
				map[m.name.GetString()] = parseRapidJson(m.value);
			}
			return Value(map);
		}

		return Value::Null;
	}

	void JsonHelper::saveValueToRapidJson(rapidjson::Value& jsonVal, const Value& val, rapidjson::Document::AllocatorType& allocator)
	{
		switch (val.getType()) {
		case Value::Type::BOOLEAN: jsonVal.SetBool(val.asBool()); break;
		case Value::Type::INTEGER: jsonVal.SetInt(val.asInt()); break;
		case Value::Type::FLOAT:   jsonVal.SetDouble(val.asFloat()); break;
		case Value::Type::DOUBLE:  jsonVal.SetDouble(val.asDouble()); break;
		case Value::Type::STRING:  jsonVal.SetString(val.asString().c_str(), allocator); break;
		case Value::Type::VECTOR: {
			if (!jsonVal.IsArray()) jsonVal.SetArray();
			for (const auto& v : val.asValueVector()) {
				rapidjson::Value child;
				saveValueToRapidJson(child, v, allocator);
				jsonVal.PushBack(child, allocator);
			}
			break;
		}
		case Value::Type::MAP: {
			if (!jsonVal.IsObject()) jsonVal.SetObject();
			for (const auto& pair : val.asValueMap()) {
				rapidjson::Value child;
				saveValueToRapidJson(child, pair.second, allocator);
				rapidjson::Value name(pair.first.c_str(), allocator);
				jsonVal.AddMember(name, child, allocator);
			}
			break;
		}
		default: break;
		}
	}

	Vec2 JsonHelper::toVec2(const Value& val)
	{
		if (val.getType() == Value::Type::MAP) {
			auto m = val.asValueMap();
			float x = m.count("x") ? m.at("x").asFloat() : 0.0f;
			float y = m.count("y") ? m.at("y").asFloat() : 0.0f;
			return Vec2(x, y);
		}
		std::string s = val.asString();
		if (s.empty()) return Vec2::ZERO;
		if (s[0] == '?') s = s.substr(1);
		return PointFromString(s);
	}

	Size JsonHelper::toSize(const Value& val)
	{
		if (val.getType() == Value::Type::MAP) {
			auto m = val.asValueMap();
			float w = m.count("width") ? m.at("width").asFloat() : 0.0f;
			float h = m.count("height") ? m.at("height").asFloat() : 0.0f;
			return Size(w, h);
		}
		std::string s = val.asString();
		if (s.empty()) return Size::ZERO;
		if (s[0] == '!') s = s.substr(1);
		return SizeFromString(s);
	}

	Color3B JsonHelper::toColor3B(const Value& val)
	{
		return Color3B::WHITE;
	}

	Color4F JsonHelper::toColor4F(const Value& val)
	{
		return Color4F::WHITE;
	}

}