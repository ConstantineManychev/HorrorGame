#ifndef __BASIC_DATA_TYPES_H__
#define __BASIC_DATA_TYPES_H__

#include "cocos2d.h"

namespace GameSpace {

	struct sMainInfo
	{
		bool isFullScreen;
		float screenHeight;
		float screenWidth;
		float spritesHeight;
		float spritesWidth;
		float scaleY;
		float scaleX;

		sMainInfo()
			: isFullScreen(false)
			, screenHeight(600.f)
			, screenWidth(800.f)
			, spritesHeight(600.f)
			, spritesWidth(800.f)
			, scaleY(1.f)
			, scaleX(1.f)
		{}
	};

	struct sSceneObjectInfo
	{
		std::string name;
		std::string type;
		cocos2d::Vec2 position;
		float scaleX;
		float scaleY;
		float rotation;
		int zOrder;
		std::string textureFileName;

		cocos2d::Value customData;

		sSceneObjectInfo()
			: scaleX(1.0f)
			, scaleY(1.0f)
			, zOrder(0)
			, rotation(0.0f)
			, customData(cocos2d::Value::Null)
		{}
	};

} // namespace GameSpace

#endif // __BASIC_DATA_TYPES_H__