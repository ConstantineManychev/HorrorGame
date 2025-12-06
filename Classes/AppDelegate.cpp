#include "AppDelegate.h"

#include "Managers/DataManager.h"
#include "Managers/GameDirector.h"
#include "Managers/InputManager.h"
#include "Managers/ViewManager.h"
#include "Managers/ScenesManager.h"
#include "Managers/EventBus.h"
#include "Managers/GameSession.h"
#include "Managers/AudioManager.h"
#include "Basics/ServiceLocator.h"
#include "Managers/CutsceneManager.h"

#include "Types/BasicDataTypes.h"
#include "Constants.h"

#include "Systems/CameraSystem.h"

#if USE_AUDIO_ENGINE
#include "audio/include/AudioEngine.h"
using namespace cocos2d::experimental;
#endif

USING_NS_CC;
_USEC;

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate()
{
	SL->unregisterService<ScenesManager>();
	SL->unregisterService<ViewManager>();
	SL->unregisterService<InputManager>();
	SL->unregisterService<GameDirector>();
	SL->unregisterService<DataManager>();
	SL->unregisterService<EventBus>();
	SL->unregisterService<GameSession>();
	SL->unregisterService<AudioManager>();

	SL->unregisterService<CameraSystem>();

	SL->unregisterService<CutsceneManager>();

	ServiceLocator::destroyInstance();

#if USE_AUDIO_ENGINE
	AudioEngine::end();
#endif
}

void AppDelegate::initGLContextAttrs()
{
	GLContextAttrs glContextAttrs = { 8, 8, 8, 8, 24, 8, 0 };

	GLView::setGLContextAttrs(glContextAttrs);
}

static int register_all_packages()
{
	return 0;
}


void AppDelegate::onInit()
{
	DM->loadMainInfo(Constants::Configs::MAIN_CONFIG);
}

bool AppDelegate::applicationDidFinishLaunching() {

	auto eventBus = EventBus::getInstance();
	SL->registerService(eventBus);

	auto dataManager = DataManager::getInstance();
	SL->registerService(dataManager);

	auto scenesManager = ScenesManager::getInstance();
	SL->registerService(scenesManager);

	auto viewManager = ViewManager::getInstance();
	SL->registerService(viewManager);

	auto inputManager = InputManager::getInstance();
	SL->registerService(inputManager);

	auto gameSession = GameSession::getInstance();
	SL->registerService(gameSession);

	auto audioManager = AudioManager::getInstance();
	SL->registerService(audioManager);

	auto gameDirector = GameDirector::getInstance();
	SL->registerService(gameDirector);

	auto cameraSystem = CameraSystem::getInstance();
	SL->registerService(cameraSystem);

	auto cutsceneManager = CutsceneManager::getInstance();
	SL->registerService(cutsceneManager);

	onInit();

	auto& mainInfo = DM->getMainInfo();

	auto director = Director::getInstance();
	auto glview = director->getOpenGLView();
	if (!glview) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)

		if (mainInfo.isFullScreen)
		{
			glview = GLViewImpl::createWithFullScreen("ABC_Adventures");
		}
		else
		{
			glview = GLViewImpl::createWithRect("ABC_Adventures", cocos2d::Rect(0, 0, mainInfo.screenWidth, mainInfo.screenHeight));
		}
#else
		glview = GLViewImpl::create("ABC_Adventures");
#endif
		director->setOpenGLView(glview);
	}

	glview->setDesignResolutionSize(mainInfo.screenWidth, mainInfo.screenHeight, ResolutionPolicy::FIXED_HEIGHT);

	DM->preloadResources();

	DM->calcScale();

	const auto& glviewFrameSize = glview->getFrameSize();

	register_all_packages();

	IM->createContext(Constants::Contexts::GAME);
	IM->createContext(Constants::Contexts::EDITOR);

#if IS_EDITOR_MODE
	GD->startEditor();
#else
	GD->runApplicationFlow();
#endif

	return true;
}

void AppDelegate::applicationDidEnterBackground() {
	Director::getInstance()->stopAnimation();

#if USE_AUDIO_ENGINE
	AudioEngine::pauseAll();
#endif
}

void AppDelegate::applicationWillEnterForeground() {
	Director::getInstance()->startAnimation();

#if USE_AUDIO_ENGINE
	AudioEngine::resumeAll();
#endif
}