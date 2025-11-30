#ifndef __EVENT_DATA_H__
#define __EVENT_DATA_H__

#include "CommonDefines.h"

_CSTART

struct EventData {
	virtual ~EventData() = default;
};

struct InputActionEventData : public EventData {
	int action;
	bool isPressed;

	InputActionEventData(int a, bool p) : action(a), isPressed(p) {}
};

struct InputAxisEventData : public EventData {
	int action;
	float value;

	InputAxisEventData(int a, float v) : action(a), value(v) {}
};

_CEND

#endif // __EVENT_DATA_H__