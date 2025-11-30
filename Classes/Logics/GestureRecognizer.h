#ifndef __GESTURE_RECOGNIZER_H__
#define __GESTURE_RECOGNIZER_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include <vector>
#include <map>

_CSTART

class GestureRecognizer
{
public:
	static GestureRecognizer* getInstance();

	char recognize(const std::vector<cocos2d::Vec2>& points);

private:
	GestureRecognizer();

	std::map<char, std::vector<cocos2d::Vec2>> mReferenceLetters;

	std::vector<cocos2d::Vec2> normalizePoints(const std::vector<cocos2d::Vec2>& aPoints);
	float calculateDistance(const cocos2d::Vec2& aPoint1, const cocos2d::Vec2& aPoint2);
	float calculateTotalDistance(const std::vector<cocos2d::Vec2>& aPoints, const std::vector<cocos2d::Vec2>& aLetterPoints);
	float roundToQuarter(float value);
};

#define GR GestureRecognizer::getInstance()

_CEND

#endif