#include "MainGameScene.h"

#include "Managers/ViewManager.h"
#include "Managers/GameDirector.h"
#include "Basics/BaseLocation.h"
#include "Systems/CameraSystem.h"
#include "Systems/LightingSystem.h"
#include "Managers/EventBus.h"
#include "Managers/EventData.h"
#include "Constants.h"

USING_NS_CC;
_CSTART

bool MainGameScene::init()
{
	if (!Parent::init()) return false;

	this->scheduleUpdate();

	mEntityCreatedListenerId = EB->subscribe<EntityCreatedEventData>(EventType::ENTITY_CREATED, [this](const EntityCreatedEventData* aData) {
		if (aData->type == Constants::Types::PLAYER)
		{
			this->setupCameraForPlayer(aData->node);
		}
	});

	Size mapSize = Size(2134, 2134);
	//LIGHT_SYS->initLighting(mapSize);

	//LIGHT_SYS->setAmbientColor(Color3B::WHITE);

	//this->addChild(LIGHT_SYS->getDarknessLayer(), 100);

	this->scheduleUpdate();

	return true;
}

void MainGameScene::useDefaultView()
{
	VM->changeView("title_view");
}

void MainGameScene::setupCameraForPlayer(Node* aPlayer)
{
	Rect worldBounds(0, 0, 2000, 2000);

	auto location = dynamic_cast<BaseLocation*>(aPlayer->getParent());
	if (location)
	{
		Size size = location->getContentSize();
		if (size.width > 0)
			worldBounds = Rect(0, 0, size.width, size.height);
	}
	else if (VM->getCurrentView())
	{
		Size size = VM->getCurrentView()->getContentSize();
		if (size.width > 0)
			worldBounds = Rect(0, 0, size.width, size.height);
	}

	CAM_SYS->init(this->getDefaultCamera(), worldBounds);
	CAM_SYS->setTarget(aPlayer);
	CAM_SYS->setDeadZoneRatios(1.0f / 3.0f, 1.0f / 3.0f);
}

void MainGameScene::update(float aDelta)
{
	CAM_SYS->update(aDelta);
	LIGHT_SYS->update(aDelta);
}

void MainGameScene::onExit()
{
	if (mEntityCreatedListenerId != 0) {
		EB->unsubscribe(EventType::ENTITY_CREATED, mEntityCreatedListenerId);
		mEntityCreatedListenerId = 0;
	}

	CAM_SYS->reset();
	Parent::onExit();
}

_CEND