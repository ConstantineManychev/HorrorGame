#ifndef __BASIC_DATA_TYPES_H__
#define __BASIC_DATA_TYPES_H__

#include "CommonDefines.h"
#include "Basics/BValue.h"

_CSTART

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

struct sWindowInfo
{
	std::string windowID;

	Value params;

	sWindowInfo()
	{}
};

struct sSceneObjectInfo
{
	std::string name;
	std::string type;
	Vec2 position;
	float scaleX;
	float scaleY;
	int zOrder;
	std::string textureFileName;
	BValue customData; // For any additional, specific data

	sSceneObjectInfo()
		: scaleX(1.0f)
		, scaleY(1.0f)
		, zOrder(0)
	{}
};


_CEND

#endif // __BASIC_DATA_TYPES_H__
