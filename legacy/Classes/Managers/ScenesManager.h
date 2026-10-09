#ifndef __SCENES_MANAGER_H__
#define __SCENES_MANAGER_H__

#include "cocos2d.h"
#include "Basics/ServiceLocator.h"
#include "Basics/BaseScene.h"
#include <map>
#include <string>
#include <functional>

_CSTART

class ScenesManager
{
	friend class AppDelegate;
public:

	using SceneCreator = std::function<BaseScene*()>;

	void registerSceneFactory(const std::string& sceneId, SceneCreator creator);
	void openScene(const std::string& aSceneID);

	BaseScene* getCurrentScene();

private:
	ScenesManager();

	std::map<std::string, BaseScene*> mScenes;
	std::map<std::string, SceneCreator> mFactories;
	BaseScene* mCurrentScene;
};

#define SM SL->getService<GameSpace::ScenesManager>()

_CEND

#endif