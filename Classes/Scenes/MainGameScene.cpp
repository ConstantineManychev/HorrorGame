#include "MainGameScene.h"

#include "Managers/ViewManager.h"

USING_NS_CC;
_CSTART

void MainGameScene::useDefaultView()
{
	VM->changeView("title");
}

_CEND
