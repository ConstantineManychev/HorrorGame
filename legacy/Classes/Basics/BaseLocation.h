#ifndef __BASE_LOCATION_H__
#define __BASE_LOCATION_H__

#include "cocos2d.h"

#include "Logics/LocationLogic.h"

_CSTART

class BaseLocation 
	: public cocos2d::Node
	, public LocationLogic
{
	typedef LocationLogic Parent;

public:

	static BaseLocation* create();

	virtual void onOpen() override;

private:

	void setupGlobalTouchListener();
	void setupGlobalKeyListener();

};

_CEND

#endif // __BASE_LOCATION_H__
