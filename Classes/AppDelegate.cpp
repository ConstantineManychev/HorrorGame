#include "AppDelegate.h"

#include "Managers/DataManager.h"
#include "Managers/GameDirector.h"
#include "Managers/InputManager.h"
#include "Managers/ViewManager.h"
#include "Managers/ScenesManager.h"
#include "Managers/EventBus.h"
#include "Basics/ServiceLocator.h"

#include "Types/BasicDataTypes.h"

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
    // Clean up services
    SL->unregisterService<ScenesManager>();
    SL->unregisterService<ViewManager>();
    SL->unregisterService<InputManager>();
    SL->unregisterService<GameDirector>();
    SL->unregisterService<DataManager>();
    SL->unregisterService<EventBus>();
    
    ServiceLocator::destroyInstance();

#if USE_AUDIO_ENGINE
    AudioEngine::end();
#endif
}

void AppDelegate::initGLContextAttrs()
{
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8, 0};

    GLView::setGLContextAttrs(glContextAttrs);
}

static int register_all_packages()
{
    return 0; //flag for packages manager
}


void AppDelegate::onInit()
{
	DM->loadMainInfo("configs/main_config.json");
}

bool AppDelegate::applicationDidFinishLaunching() {

    // --- Initialize Service Locator & Services ---
    
    // EventBus
    auto eventBus = EventBus::getInstance(); 
    SL->registerService(eventBus);

    // DataManager
    auto dataManager = DataManager::getInstance();
    SL->registerService(dataManager);

    // ScenesManager
    auto scenesManager = ScenesManager::getInstance();
    SL->registerService(scenesManager);

    // ViewManager
    auto viewManager = ViewManager::getInstance();
    SL->registerService(viewManager);

    // InputManager
    auto inputManager = InputManager::getInstance();
    SL->registerService(inputManager);

    // GameDirector
    auto gameDirector = GameDirector::getInstance();
    SL->registerService(gameDirector);

    // --- Initialization ---

	onInit();

	auto& mainInfo = DM->getMainInfo();

    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
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


	DM->calcScale();

    const auto& glviewFrameSize = glview->getFrameSize();

    register_all_packages();
    
    // Setup Input Contexts
    IM->createContext("Game");
    IM->createContext("Editor");
    // Bindings are set in InputManager ctor, but can be overridden here

#if IS_EDITOR_MODE
	GD->startEditor();
#else
	GD->startGame();
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
