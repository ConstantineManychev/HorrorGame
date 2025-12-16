#include "AppDelegate.h"
#include "Basics/ServiceLocator.h"
#include "Constants.h"

#include "Managers/ConfigLoader.h"
#include "Managers/DataManager.h"
#include "Managers/GameDirector.h"
#include "Managers/ScenesManager.h"
#include "Managers/InputManager.h"
#include "Managers/ViewManager.h"
#include "Managers/EventBus.h"
#include "Managers/AudioManager.h"
#include "Managers/CutsceneManager.h"
#include "Managers/GameSession.h"
#include "Factories/ViewFactory.h"
#include "Factories/EntityFactory.h"
#include "Systems/CameraSystem.h"
#include "Systems/LightingSystem.h"

#include "Scenes/MainGameScene.h"
#include "Scenes/EditorScene.h"

USING_NS_CC;
_USEC;

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate()
{
	auto lightSys = SL->getService<LightingSystem>();
	if (lightSys) lightSys->release();
	SL->unregisterService<LightingSystem>();

	delete SL->getService<CameraSystem>();
	SL->unregisterService<CameraSystem>();

	delete SL->getService<CutsceneManager>();
	SL->unregisterService<CutsceneManager>();

	delete SL->getService<GameDirector>();
	SL->unregisterService<GameDirector>();

	delete SL->getService<AudioManager>();
	SL->unregisterService<AudioManager>();

	delete SL->getService<GameSession>();
	SL->unregisterService<GameSession>();

	delete SL->getService<InputManager>();
	SL->unregisterService<InputManager>();

	delete SL->getService<ViewManager>();
	SL->unregisterService<ViewManager>();

	delete SL->getService<ScenesManager>();
	SL->unregisterService<ScenesManager>();

	delete SL->getService<ViewFactory>();
	SL->unregisterService<ViewFactory>();

	delete SL->getService<EntityFactory>();
	SL->unregisterService<EntityFactory>();

	delete SL->getService<ConfigLoader>();
	SL->unregisterService<ConfigLoader>();

	delete SL->getService<DataManager>();
	SL->unregisterService<DataManager>();

	delete SL->getService<EventBus>();
	SL->unregisterService<EventBus>();

	ServiceLocator::destroyInstance();
}

void AppDelegate::initGLContextAttrs()
{
	GLContextAttrs glContextAttrs = { 8, 8, 8, 8, 24, 8, 0 };
	GLView::setGLContextAttrs(glContextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
	auto director = Director::getInstance();
	auto glview = director->getOpenGLView();


	SL->registerService(new EventBus());
	SL->registerService(new DataManager());
	SL->registerService(new ConfigLoader());

	SL->registerService(new EntityFactory());
	SL->registerService(new ViewFactory());

	SL->registerService(new ScenesManager());
	SL->registerService(new ViewManager());
	SL->registerService(new InputManager());
	SL->registerService(new GameSession());
	SL->registerService(new AudioManager());

	SL->registerService(new CameraSystem());
	SL->registerService(new LightingSystem());
	SL->registerService(new CutsceneManager());

	SL->registerService(new GameDirector());

	CL->loadMainConfig(Constants::Configs::MAIN_CONFIG);
	CL->loadViewConfigs("configs/views/views_list.json");

	auto& mainInfo = DM->getMainInfo();

	if (!glview) {
		if (mainInfo.isFullScreen) glview = GLViewImpl::createWithFullScreen("ABC_Adventures");
		else glview = GLViewImpl::createWithRect("ABC_Adventures", Rect(0, 0, mainInfo.screenWidth, mainInfo.screenHeight));
		director->setOpenGLView(glview);
	}

	auto listener = EventListenerCustom::create("glview_window_resized", [=](EventCustom* e) {
		auto view = Director::getInstance()->getOpenGLView();
		Size newSize = view->getFrameSize();
		if (newSize.height > 0) {
			float designH = 600.0f;
			float designW = designH * (newSize.width / newSize.height);
			view->setDesignResolutionSize(designW, designH, ResolutionPolicy::FIXED_HEIGHT);
			if (DM) DM->updateScreenSize(Size(designW, designH));
		}
	});
	director->getEventDispatcher()->addEventListenerWithFixedPriority(listener, 1);

	glview->setDesignResolutionSize(mainInfo.screenWidth, mainInfo.screenHeight, ResolutionPolicy::FIXED_HEIGHT);
	DM->calcScale();

	DM->preloadResources();

	SM->registerSceneFactory(Constants::Scenes::TITLE, []() { return MainGameScene::create(); });
	SM->registerSceneFactory(Constants::Scenes::GAME, []() { return MainGameScene::create(); });
	SM->registerSceneFactory(Constants::Scenes::EDITOR, []() { return EditorScene::create(); });

	IM->createContext(Constants::Contexts::GAME);

#if IS_EDITOR_MODE
	GD->startEditor();
#else
	GD->runApplicationFlow();
#endif

	return true;
}

void AppDelegate::applicationDidEnterBackground() { Director::getInstance()->stopAnimation(); }
void AppDelegate::applicationWillEnterForeground() { Director::getInstance()->startAnimation(); }