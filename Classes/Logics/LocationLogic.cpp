#include "LocationLogic.h"

#include "Managers/ViewManager.h"
#include "Managers/DataManager.h"
#include "Managers/GameDirector.h"

_USEC

static const int TAIL_TAG = 99;

LocationLogic::LocationLogic()
	: mTaleParticles(nullptr)
	, mPlayer(nullptr)
	, mIsCurrentlyDrawing(false)
{
}

LocationLogic::~LocationLogic()
{
	GD->setLocation(nullptr);
	GD->setPlayer(nullptr);
}

void LocationLogic::onOpen()
{
	GD->setPlayer(mPlayer);
}
	

void LocationLogic::onButtonTouchEvent(Ref* aSender, Touch* aTouch, ui::Widget::TouchEventType aEventType)
{
	Node* node = dynamic_cast<Node*>(aSender);

	if (node)
	{
		switch (aEventType)
		{
			case ui::Widget::TouchEventType::BEGAN:
			{
				onButtonTouchBegin(node, aTouch);
				break;
			}
			case ui::Widget::TouchEventType::CANCELED:
			{
				onButtonTouchCanceled(node, aTouch);
				break;
			}
			case ui::Widget::TouchEventType::ENDED:
			{
				onButtonTouchFinish(node, aTouch);
				break;
			}
			case ui::Widget::TouchEventType::MOVED:
			{
				onButtonTouchMove(node, aTouch);
				break;
			}
		}
	}
}

void LocationLogic::onButtonTouchBegin(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, "onBtnClickDown");

		if (aTouch)
		{
			mIsCurrentlyDrawing = true;
			std::string particlePath;
			const auto& path = DM->getGlobalValue("tale_paticle_path");
			if (path.isString())
			{
				particlePath = path.getString();
			}

			mTaleParticles = ParticleSystemQuad::create(particlePath);

			auto location = aTouch->getLocation();

			if (mTaleParticles)
			{
				mTaleParticles->setLocalZOrder(1000);
				mTaleParticles->setTag(TAIL_TAG);
				mTaleParticles->setPosition(aNode->convertToNodeSpace(location));
				aNode->addChild(mTaleParticles);
			}

			mTouches.push_back(location);
		}
	}
}

void LocationLogic::onButtonTouchFinish(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, "onBtnClickUp");

		char recognizedLetter = recognizeLetter(mTouches);

		if (recognizedLetter == '?')
		{
			if (mTaleParticles)
			{
				float delayTime = mTaleParticles->getLife();

				auto delay = DelayTime::create(delayTime);

				Node* taleNode = mTaleParticles;

				auto removeParticles = CallFunc::create([=]()
				{
					aNode->removeChild(taleNode);

					auto child = aNode->getChildByTag(TAIL_TAG);
					if (!child && !mIsCurrentlyDrawing)
					{
						mTaleParticles = nullptr;
						mTouches.clear();
					}
				});

				auto sequence = Sequence::create(delay, removeParticles, nullptr);
				sequence->setTag(TAIL_TAG);

				mTaleParticles->stop();
				aNode->runAction(sequence);
			}
		}
		else
		{
			while ( aNode->getChildByTag(TAIL_TAG) )
			{
				aNode->removeChildByTag(TAIL_TAG);
			}
			mTaleParticles = nullptr;
			mTouches.clear();
		}
	}

	mIsCurrentlyDrawing = false;
}

void LocationLogic::onButtonTouchCanceled(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, "onBtnClickCanceled");
	}
}

void LocationLogic::onButtonTouchMove(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, "onBtnClickMove");

		if (aTouch && mTaleParticles)
		{
			auto location = aTouch->getLocation();

			mTaleParticles->setPosition(aNode->convertToNodeSpace(location));

			if (calculateDistance(mTouches.back(), location) >= 100.f)
			{
				mTouches.push_back(location);
			}
		}
	}
}

void LocationLogic::onKeyDown(Ref* aSender, EventKeyboard::KeyCode aKeyCode)
{
	Node* node = dynamic_cast<Node*>(aSender);

	if (mPlayer)
	{
		if (aKeyCode == DM->getKey("left"))
		{
			mPlayer->stopAllActions();
			//auto repeatForeverAction = RepeatForever::create( MoveBy::create(1.f,Vec2(10.f, 0.f) ) );

			//mPlayer->runAction(repeatForeverAction);

			mPlayer->addForceX(-500.f);
		}
		else if (aKeyCode == DM->getKey("right"))
		{
			mPlayer->stopAllActions();

			mPlayer->addForceX(500.f);
		}

		if (aKeyCode == DM->getKey("up"))
		{
			mPlayer->stopAllActions();

			mPlayer->addForceY(500.f);
		}
		else if (aKeyCode == DM->getKey("down"))
		{
			mPlayer->stopAllActions();

			mPlayer->addForceY(-500.f);
		}
	}
}

void LocationLogic::onKeyUp(Ref* aSender, EventKeyboard::KeyCode aKeyCode)
{
	if (mPlayer)
	{
		if (aKeyCode == DM->getKey("left"))
		{
			mPlayer->addForceX(500.f);
		}
		else if (aKeyCode == DM->getKey("right"))
		{
			mPlayer->addForceX(-500.f);
		}

		if (aKeyCode == DM->getKey("up"))
		{
			mPlayer->addForceY(-500.f);
		}
		else if (aKeyCode == DM->getKey("down"))
		{
			mPlayer->addForceY(500.f);
		}
	}
}

void LocationLogic::setupPlayer(Node* aNode)
{
	if (aNode)
	{
		mPlayer = dynamic_cast<Player*>(aNode);
	}
}

float LocationLogic::roundToQuarter(float value) 
{
	return std::round(value * 4) / 4.0f;
}

std::vector<Vec2> LocationLogic::normalizePoints(const std::vector<Vec2>& aPoints)
{
	std::vector<Vec2> normalizedPoints;
	if (!aPoints.empty())
	{
		float minX = aPoints[0].x, minY = aPoints[0].y;
		float maxX = minX, maxY = minY;

		for (const auto& point : aPoints)
		{
			minX = std::min(minX, point.x);
			minY = std::min(minY, point.y);
			maxX = std::max(maxX, point.x);
			maxY = std::max(maxY, point.y);
		}

		std::set<Vec2> uniqueNormalizedPoints;

		for (const auto& point : aPoints)
		{
			float normX = (point.x - minX) / (maxX - minX);
			float normY = (point.y - minY) / (maxY - minY);

			normX = roundToQuarter(normX);
			normY = roundToQuarter(normY);

			uniqueNormalizedPoints.emplace(normX, normY);
		}

		normalizedPoints = std::vector<Vec2>(uniqueNormalizedPoints.begin(), uniqueNormalizedPoints.end());
	}

	return normalizedPoints;
}

float LocationLogic::calculateDistance(const Vec2& aPoint1, const Vec2& aPoint2)
{
	float dx = aPoint1.x - aPoint2.x;
	float dy = aPoint1.y - aPoint2.y;
	return std::sqrt(dx * dx + dy * dy);
}

float LocationLogic::calculateTotalDistance(const std::vector<Vec2>& aPoint, const std::vector<Vec2>& aLetterPoints) {
	float totalDistance = 0.0f;

	for (const auto& letterPoint : aLetterPoints)
	{
		float letterPointDistance = std::numeric_limits<float>::max();

		for (const auto& point : aPoint)
		{
			auto distance = calculateDistance(point, letterPoint);

			if (distance < letterPointDistance)
			{
				letterPointDistance = distance;
			}
		}

		totalDistance += letterPointDistance;
	}

	return totalDistance / aLetterPoints.size();
}

bool LocationLogic::checkLetter(const std::vector<Vec2>& aPoints, const std::vector<Vec2>& aLetterPoints)
{
	bool result = !aLetterPoints.empty();

	for (auto& point : aLetterPoints)
	{
		if (std::find(aPoints.begin(),aPoints.end(), point) == aPoints.end())
		{
			result = false;
			break;
		}
	}

	return result;
}

char LocationLogic::recognizeLetter(const std::vector<Vec2>& aPoints)
{
	std::vector<Vec2> normalizedInput = normalizePoints(aPoints);

	char bestMatch = '?';
	float minDistance = 0.08f;

	for (const auto& ref : referenceLetters) 
	{
		float distance = calculateTotalDistance(normalizedInput, ref.second);
		if (distance < minDistance) 
		{
			minDistance = distance;
			bestMatch = ref.first;
		}
	}

	CCLOG("The letter is: %c", bestMatch);
	createLetter(bestMatch, Vec2::ZERO);

	return bestMatch;
}

void LocationLogic::createLetter(char aLetter, const Vec2& aPoint)
{
	if (aLetter != '?')
	{
		std::string spriteName = "res/main_menu/MM_" + std::string(1, aLetter) + "_Letter.png";

		auto mainInfo = DM->getMainInfo();
		auto heigh = mainInfo.screenHeight;
		auto width = mainInfo.screenWidth;

		Sprite* sprite = Sprite::create();
		sprite->initWithFile(spriteName);

		auto loc = GD->getCurrentLocation();

		loc->addChild(sprite);
		sprite->setPosition(width / 2, heigh / 2);
		sprite->setLocalZOrder(loc->getLocalZOrder() + 10);
	}
}