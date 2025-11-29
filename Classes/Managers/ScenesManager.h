#ifndef __SCENES_MANAGER_H__
#define __SCENES_MANAGER_H__

#include "CommonDefines.h"
#include "Basics/BaseScene.h"
#include "Basics/ServiceLocator.h" // Include

_CSTART

class ScenesManager
{
    friend class ServiceLocator;
    friend class AppDelegate;

private:

	ScenesManager();

	std::map<std::string, BaseScene*> mScenes;

	std::string mCurrentSceneID;
	BaseScene* mCurrentScene;

public:

	static ScenesManager* getInstance();

	void registerScene(const std::string& aSceneID, BaseScene* aScene);
	void openScene(const std::string& aSceneID);

	void runEditorScene();
	void runGameScene();

	BaseScene* getCurrentScene();
	std::string& getCurrentSceneID();

};

#define SM SL->getService<ScenesManager>()

_CEND

#endif // __SCENES_MANAGER_H__
