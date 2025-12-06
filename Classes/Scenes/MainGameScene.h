#ifndef __MAIN_GAME_SCENE_H__
#define __MAIN_GAME_SCENE_H__

#include "Basics/BaseScene.h"
#include "CommonDefines.h"

_CSTART

class MainGameScene : public BaseScene
{
	typedef BaseScene Parent;

public:

	bool init() override;
	void useDefaultView() override;

	virtual void update(float aDelta) override;
	virtual void onExit() override;

	CREATE_FUNC(MainGameScene);

private:
	size_t mEntityCreatedListenerId = 0;
	void setupCameraForPlayer(cocos2d::Node* aPlayer);
};

_CEND

#endif