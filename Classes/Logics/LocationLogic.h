#ifndef __LOCATION_LOGIC_H__
#define __LOCATION_LOGIC_H__

#include "cocos2d.h"
#include "ui/UIWidget.h"
#include "Basics/Player.h"
#include "CommonDefines.h"

_CSTART

class LocationLogic
{
private:
	std::vector< cocos2d::Vec2 > mTouches;
	cocos2d::ParticleSystemQuad* mTaleParticles;
	Player* mPlayer;
	bool mIsCurrentlyDrawing;

	virtual void onButtonTouchBegin(cocos2d::Node* aNode, cocos2d::Touch* aTouch);
	virtual void onButtonTouchFinish(cocos2d::Node* aNode, cocos2d::Touch* aTouch);
	virtual void onButtonTouchCanceled(cocos2d::Node* aNode, cocos2d::Touch* aTouch);
	virtual void onButtonTouchMove(cocos2d::Node* aNode, cocos2d::Touch* aTouch);

	void createLetter(char aLetter, const cocos2d::Vec2& aPoint);

protected:
	virtual void onKeyDown(cocos2d::Ref* aSender, cocos2d::EventKeyboard::KeyCode aKeyCode);
	virtual void onKeyUp(cocos2d::Ref* aSender, cocos2d::EventKeyboard::KeyCode aKeyCode);

public:
	LocationLogic();
	~LocationLogic();

	void onButtonTouchEvent(cocos2d::Ref* aSender, cocos2d::Touch* touch, cocos2d::ui::Widget::TouchEventType aEventType);
	void setupPlayer(cocos2d::Node* aNode);
	virtual void onOpen();
};

_CEND

#endif