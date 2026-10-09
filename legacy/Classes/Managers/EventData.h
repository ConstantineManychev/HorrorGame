#ifndef __EVENT_DATA_H__
#define __EVENT_DATA_H__

#include "CommonDefines.h"
#include "cocos2d.h"

_CSTART

struct EventData {
	virtual ~EventData() = default;
};

struct InputActionEventData : public EventData {
	int action;
	bool isPressed;

	InputActionEventData(int aAction, bool aIsPressed) : action(aAction), isPressed(aIsPressed) {}
};

struct InputAxisEventData : public EventData {
	int action;
	float value;

	InputAxisEventData(int aAction, float aValue) : action(aAction), value(aValue) {}
};

struct EntityCreatedEventData : public EventData {
	cocos2d::Node* node;
	std::string type;

	EntityCreatedEventData(cocos2d::Node* aNode, const std::string& aType) : node(aNode), type(aType) {}
};

_CEND

#endif