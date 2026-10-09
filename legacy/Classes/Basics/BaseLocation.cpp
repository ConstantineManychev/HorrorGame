#include "BaseLocation.h"
#include "Managers/GameDirector.h"

USING_NS_CC;
_CSTART

BaseLocation* BaseLocation::create()
{
	BaseLocation* ret = new (std::nothrow) BaseLocation();
	if (ret && ret->init())
	{
		ret->autorelease();
        
        if (!GD->isEditorMode()) // Disable logic listeners in editor
        {
            ret->setupGlobalTouchListener();
            ret->setupGlobalKeyListener();
        }
		return ret;
	}
	else
	{
		delete ret;
		return nullptr;
	}
}

void BaseLocation::onOpen()
{
    if (GD->isEditorMode()) return; // Disable logic in editor
    
	Parent::onOpen();
	//GD->setLocation(this);
}

void BaseLocation::setupGlobalTouchListener()
{
	auto touchListener = EventListenerTouchAllAtOnce::create();

	touchListener->onTouchesBegan = [this](const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
	{
		for (auto touch : touches)
		{
			this->onButtonTouchEvent(this, touch, cocos2d::ui::Widget::TouchEventType::BEGAN);
		}
		return true;
	};

	touchListener->onTouchesMoved = [this](const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
	{
		for (auto touch : touches)
		{
			this->onButtonTouchEvent(this, touch, cocos2d::ui::Widget::TouchEventType::MOVED);
		}
	};

	touchListener->onTouchesEnded = [this](const std::vector<Touch*>& touches, cocos2d::Event* event)
	{
		for (auto touch : touches)
		{
			this->onButtonTouchEvent(this, touch, cocos2d::ui::Widget::TouchEventType::ENDED);
		}
	};

	touchListener->onTouchesCancelled = [this](const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
	{
		for (auto touch : touches)
		{
			this->onButtonTouchEvent(this, touch, cocos2d::ui::Widget::TouchEventType::CANCELED);
		}
	};

	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

void BaseLocation::setupGlobalKeyListener()
{
	auto listener = cocos2d::EventListenerKeyboard::create();

	listener->onKeyPressed = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event)
	{
		this->onKeyDown(this, keyCode);
	};

	listener->onKeyReleased = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event)
	{
		this->onKeyUp(this, keyCode);
	};

	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
}

_CEND