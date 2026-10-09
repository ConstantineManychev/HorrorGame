#include "ScenesManager.h"

USING_NS_CC;
_CSTART

ScenesManager::ScenesManager() : mCurrentScene(nullptr) {}

void ScenesManager::registerSceneFactory(const std::string& sceneId, SceneCreator creator)
{
	mFactories[sceneId] = creator;
}

void ScenesManager::openScene(const std::string& aSceneID)
{
	auto it = mScenes.find(aSceneID);
	BaseScene* sceneToOpen = nullptr;

	if (it != mScenes.end())
	{
		sceneToOpen = it->second;
	}
	else
	{
		auto factoryIt = mFactories.find(aSceneID);
		if (factoryIt != mFactories.end())
		{
			sceneToOpen = factoryIt->second();
			mScenes[aSceneID] = sceneToOpen;
		}
	}

	if (sceneToOpen)
	{
		if (mCurrentScene == sceneToOpen) return;

		Director::getInstance()->replaceScene(sceneToOpen);
		mCurrentScene = sceneToOpen;
	}
	else
	{
		CCLOG("ScenesManager: Scene not found and no factory registered for %s", aSceneID.c_str());
	}
}

BaseScene* ScenesManager::getCurrentScene()
{
	return mCurrentScene;
}

_CEND