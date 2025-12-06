#include "LocationLogic.h"
#include "Managers/ViewManager.h"
#include "Managers/DataManager.h"
#include "Managers/GameDirector.h"
#include "Logics/GestureRecognizer.h"
#include "Constants.h"

USING_NS_CC;
using namespace GameSpace;

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
			onButtonTouchBegin(node, aTouch);
			break;
		case ui::Widget::TouchEventType::CANCELED:
			onButtonTouchCanceled(node, aTouch);
			break;
		case ui::Widget::TouchEventType::ENDED:
			onButtonTouchFinish(node, aTouch);
			break;
		case ui::Widget::TouchEventType::MOVED:
			onButtonTouchMove(node, aTouch);
			break;
		}
	}
}

void LocationLogic::onButtonTouchBegin(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, Constants::Actions::BTN_CLICK_DOWN);

		if (aTouch)
		{
			mIsCurrentlyDrawing = true;
			std::string particlePath;
			const auto& pathVal = DM->getGlobalValue(Constants::Keys::PARTICLE_PATH);

			if (pathVal.getType() == Value::Type::STRING)
			{
				particlePath = pathVal.asString();
			}

			if (!particlePath.empty()) {
				mTaleParticles = ParticleSystemQuad::create(particlePath);
			}

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
		VM->runActionForNode(aNode, Constants::Actions::BTN_CLICK_UP);

		char recognizedLetter = GR->recognize(mTouches);
		CCLOG("Gesture Recognized: %c", recognizedLetter);

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
			while (aNode->getChildByTag(TAIL_TAG))
			{
				aNode->removeChildByTag(TAIL_TAG);
			}
			mTaleParticles = nullptr;
			mTouches.clear();

			createLetter(recognizedLetter, Vec2::ZERO);
		}
	}
	mIsCurrentlyDrawing = false;
}

void LocationLogic::onButtonTouchCanceled(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, Constants::Actions::BTN_CLICK_CANCEL);
	}
}

void LocationLogic::onButtonTouchMove(Node* aNode, Touch* aTouch)
{
	if (aNode)
	{
		VM->runActionForNode(aNode, Constants::Actions::BTN_CLICK_MOVE);

		if (aTouch && mTaleParticles)
		{
			auto location = aTouch->getLocation();
			mTaleParticles->setPosition(aNode->convertToNodeSpace(location));

			if (mTouches.empty() || mTouches.back().distance(location) >= 20.f)
			{
				mTouches.push_back(location);
			}
		}
	}
}

void LocationLogic::onKeyDown(Ref* aSender, EventKeyboard::KeyCode aKeyCode)
{
}

void LocationLogic::onKeyUp(Ref* aSender, EventKeyboard::KeyCode aKeyCode)
{
}

void LocationLogic::setupPlayer(Node* aNode)
{
	if (aNode)
	{
		mPlayer = dynamic_cast<Player*>(aNode);
	}
}

void LocationLogic::createLetter(char aLetter, const Vec2& aPoint)
{
	if (aLetter != '?')
	{
		std::string spriteName = "res/main_menu/MM_" + std::string(1, aLetter) + "_Letter.png";
		auto mainInfo = DM->getMainInfo();

		Sprite* sprite = Sprite::create();
		if (FileUtils::getInstance()->isFileExist(spriteName)) {
			sprite->initWithFile(spriteName);
		}
		else {
			sprite = Sprite::create("HelloWorld.png");
			sprite->setColor(Color3B::MAGENTA);
		}

		auto loc = GD->getCurrentLocation();
		if (loc)
		{
			loc->addChild(sprite);
			sprite->setPosition(mainInfo.screenWidth / 2, mainInfo.screenHeight / 2);
			sprite->setLocalZOrder(loc->getLocalZOrder() + 10);
		}
	}
}