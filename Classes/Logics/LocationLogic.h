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

	std::map<char, std::vector<Vec2>> referenceLetters = {
		{'A', { {0, 0}, {0.5, 1}, {1, 0}, {0.5, 0.5} } },
		{'B', { {0, 0}, {0, 1}, {1, 0.75}, {1, 0.25} } },
		{'C', { {0.5, 0}, {0.5, 1}, {0, 0.5} } },
		{'D', { {0, 0}, {0, 1}, {1, 0.5} } },
		{'E', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5}, {1, 0} } },
		{'F', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5} } },
		{'G', { {0, 0}, {0, 1}, {1, 1}, {1, 0.75}, {0.5, 0.5} } },
		{'H', { {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0.5, 0.5} } },
		{'I', { {0.5, 0}, {0.5, 1} } },
		{'J', { {1, 0}, {0.5, 0}, {0.5, 1}, {0, 1}, {0, 0.5} } },
		{'K', { {0, 0}, {0, 1}, {1, 0.5}, {0, 0.75}, {1, 0.25} } },
		{'L', { {0, 0}, {0, 1}, {1, 1} } },
		{'M', { {0, 0}, {0, 1}, {0.5, 0.5}, {1, 1}, {1, 0} } },
		{'N', { {0, 0}, {0, 1}, {1, 0}, {1, 1}, {0.5, 0.5}, {0.75, 0.25}, {0.25, 0.75} } },
		{'O', { {0.5, 0}, {0, 0.5}, {0.5, 1}, {1, 0.5} } },
		{'P', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5}, {0, 0.5} } },
		{'Q', { {0.5, 0}, {0, 0.5}, {0.5, 1}, {1, 0.5}, {0.5, 0.25} } },
		{'R', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5}, {0, 0.5}, {1, 0} } },
		{'S', { {0.5, 0}, {0, 0.5}, {0.5, 1}, {1, 0.5}, {0.5, 0.25} } },
		{'T', { {0, 1}, {1, 1}, {0.5, 0} } },
		{'U', { {0, 0}, {0, 1}, {1, 1}, {1, 0} } },
		{'V', { {0, 1}, {0.5, 0}, {1, 1} } },
		{'W', { {0, 1}, {0.5, 0}, {1, 1}, {1, 0}, {0.5, 0.5} } },
		{'X', { {0, 0}, {1, 1}, {1, 0}, {0, 1} } },
		{'Y', { {0, 1}, {0.5, 0}, {1, 1}, {0.5, 0.5} } },
		{'Z', { {0, 1}, {1, 1}, {0, 0}, {1, 0} } }
	};

	std::vector< Vec2 > mTouches;

	ParticleSystemQuad* mTaleParticles;
	Player* mPlayer;

	bool mIsCurrentlyDrawing;

	virtual void onButtonTouchBegin(Node* aNode, Touch* aTouch);
	virtual void onButtonTouchFinish(Node* aNode, Touch* aTouch);
	virtual void onButtonTouchCanceled(Node* aNode, Touch* aTouch);
	virtual void onButtonTouchMove(Node* aNode, Touch* aTouch);

	float roundToQuarter(float value);
	std::vector<Vec2> normalizePoints(const std::vector<Vec2>& aPoints);
	float calculateDistance(const Vec2& aPoint1, const Vec2& aPoint2);
	float calculateTotalDistance(const std::vector<Vec2>& aPoints, const std::vector<Vec2>& aLetterPoints);
	bool checkLetter(const std::vector<Vec2>& aPoints, const std::vector<Vec2>& aLetterPoints);
	char recognizeLetter(const std::vector<Vec2>& aPoints);

	void createLetter(char aLetter, const Vec2& aPoint);

protected:

	virtual void onKeyDown(Ref* aSender, EventKeyboard::KeyCode aKeyCode);
	virtual void onKeyUp(Ref* aSender, EventKeyboard::KeyCode aKeyCode);

public:

	void onButtonTouchEvent(Ref* aSender, Touch* touch, ui::Widget::TouchEventType aEventType);
	void setupPlayer(Node* aNode);

	virtual void onOpen();

	LocationLogic();
	~LocationLogic();
};

_CEND

#endif // __LOCATION_LOGIC_H__
