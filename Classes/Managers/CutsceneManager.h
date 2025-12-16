#ifndef __CUTSCENE_MANAGER_H__
#define __CUTSCENE_MANAGER_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Basics/ServiceLocator.h"
#include <queue>
#include <functional>
#include <string>

_CSTART

struct CutsceneStep
{
	std::string type;
	cocos2d::ValueMap params;
};

class CutsceneManager
{
	friend class ServiceLocator;
	friend class AppDelegate;

public:

	void playCutscene(const std::string& aConfigPath, std::function<void()> onComplete = nullptr);

	void stopCutscene();

	bool isPlaying() const { return mIsPlaying; }

private:
	CutsceneManager();
	~CutsceneManager();

	void parseCutsceneConfig(const std::string& aConfigPath);
	void processNextStep();
	void finishCutscene();

	// Handlers
	void handleWait(const cocos2d::ValueMap& params);
	void handleCameraMove(const cocos2d::ValueMap& params);
	void handleCameraZoom(const cocos2d::ValueMap& params);
	void handleNodeAction(const cocos2d::ValueMap& params);
	void handleChangeView(const cocos2d::ValueMap& params);
	void handlePlaySound(const cocos2d::ValueMap& params);

	// Helpers
	cocos2d::Node* findNodeInScene(const std::string& name);
	cocos2d::Node* findNodeRecursive(cocos2d::Node* root, const std::string& name);

	std::queue<CutsceneStep> mStepsQueue;
	bool mIsPlaying;
	std::function<void()> mOnCompleteCallback;
};

#define CSM SL->getService<CutsceneManager>()

_CEND

#endif // __CUTSCENE_MANAGER_H__