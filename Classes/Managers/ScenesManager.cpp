#include "ScenesManager.h"

#include "DataManager.h"
#include "Scenes/EditorScene.h"
#include "Scenes/MainGameScene.h"
#include "ui/UIButton.h"
#include "CommonDefines.h"
#include "Managers/ViewManager.h"
#include "Constants.h"

#include "Helpers/NodeHelper.h"

USING_NS_CC;
_CSTART

ScenesManager::ScenesManager()
	: mCurrentScene(nullptr)
{

}

ScenesManager* ScenesManager::getInstance()
{
	static ScenesManager instance;
	return &instance;
}

void ScenesManager::openScene(const std::string& aSceneID)
{
	if (mCurrentScene)
	{
		mCurrentScene->removeAllChildren();
	}
	auto sceneIt = mScenes.find(aSceneID);

	if (sceneIt != mScenes.end())
	{
		mCurrentSceneID = aSceneID;
		mCurrentScene = sceneIt->second;

		if (Director::getInstance()->getRunningScene())
		{
			Director::getInstance()->replaceScene(mCurrentScene);
		}
		else
		{
			Director::getInstance()->runWithScene(mCurrentScene);
		}

		auto currentView = mCurrentScene->getCurrentViewID();

		if (currentView.empty())
		{
			mCurrentScene->useDefaultView();
		}
		else
		{
			VM->changeView(currentView);
		}

	}
}

void ScenesManager::registerScene(const std::string& aSceneID, BaseScene* aScene)
{
	mScenes[aSceneID] = aScene;
}

void ScenesManager::runEditorScene()
{
	CCLOG("ScenesManager: Running Editor Scene.");
	std::string editorSceneID = Constants::Scenes::EDITOR;

	std::string currentViewID = Constants::Scenes::MAIN;
	if (mCurrentScene && !mCurrentScene->getCurrentViewID().empty())
	{
		currentViewID = mCurrentScene->getCurrentViewID();
	}

	auto sceneIt = mScenes.find(editorSceneID);

	if (sceneIt == mScenes.end())
	{
		auto scene = EditorScene::createScene(currentViewID);
		registerScene(editorSceneID, scene);
	}

	openScene(editorSceneID);
}

void ScenesManager::runGameScene()
{
	CCLOG("ScenesManager: Running Main Game Scene.");
	std::string mainSceneID = Constants::Scenes::MAIN;

	auto sceneIt = mScenes.find(mainSceneID);

	if (sceneIt == mScenes.end())
	{
		auto scene = MainGameScene::create();
		registerScene(mainSceneID, scene);
	}

	openScene(mainSceneID);
}

BaseScene* ScenesManager::getCurrentScene()
{
	return mCurrentScene;
}

std::string& ScenesManager::getCurrentSceneID()
{
	return mCurrentSceneID;
}

_CEND