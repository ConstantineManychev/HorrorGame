#include "ScenesManager.h"

#include "DataManager.h"
#include "Scenes/EditorScene.h"
#include "Scenes/MainGameScene.h"
#include "ui/UIButton.h"
#include "CommonDefines.h"
#include "Managers/ViewManager.h"

#include "Helpers/NodeHelper.h"

_USEC

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

#if DEVELOP_MODE == 1
			if (mCurrentSceneID != "editor")
			{
				auto editButton = ui::Button::create();
				editButton->setTitleText("Edit Scene");
				editButton->setTitleFontSize(24);

				auto visibleSize = Director::getInstance()->getVisibleSize();
				editButton->setPosition(Vec2(visibleSize.width - editButton->getContentSize().width / 2 - 10, visibleSize.height - 30));
				editButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type) {
					if (type == ui::Widget::TouchEventType::ENDED)
					{
						SM->runEditorScene();
					}
				});
				mCurrentScene->addChild(editButton, 9999);
			}
#endif
		}
		else
		{
			VM->changeView(currentView);
		}

	}
	else
	{

	}
}

void ScenesManager::registerScene(const std::string& aSceneID, BaseScene* aScene)
{
	mScenes[aSceneID] = aScene;
}

void ScenesManager::runEditorScene()
{
	CCLOG("ScenesManager: Running Editor Scene."); 
	std::string editorSceneID = "editor";

	std::string currentViewID = "main";
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
	std::string mainSceneID = "main";

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