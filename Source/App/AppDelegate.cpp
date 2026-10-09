#include "AppDelegate.h"

#include "Launch.h"

#include "Runtime/App/AppContext.h"
#include "Runtime/App/Log.h"

using namespace ax;

AppDelegate::AppDelegate() = default;

AppDelegate::~AppDelegate()
{
    mContext.reset();
}

void AppDelegate::initGfxContextAttrs()
{
    GfxContextAttrs attributes = {8, 8, 8, 8, 24, 8, 0};
    RenderView::setGfxContextAttrs(attributes);
}

bool AppDelegate::applicationDidFinishLaunching()
{
    mContext = std::make_unique<hg::AppContext>();
    hg::prepareContent(*mContext);

    hg::Diagnostics diagnostics;
    mContext->initialize(diagnostics);
    hg::Log::diagnostics(diagnostics);

    const hg::WindowSettings window = hg::windowSettings(mContext->config());
    auto* director = Director::getInstance();
    auto* renderView = director->getRenderView();
    if (!renderView)
    {
#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID) && (AX_TARGET_PLATFORM != AX_PLATFORM_IOS)
        if (window.fullscreen)
        {
            renderView = RenderViewImpl::createWithFullScreen(window.title);
        }
        else
        {
            renderView = RenderViewImpl::createWithRect(window.title, Rect(0.0f, 0.0f, window.size.x, window.size.y), 1.0f, window.resizable);
        }
#else
        renderView = RenderViewImpl::create(window.title);
#endif
        director->setRenderView(renderView);
    }

    director->setStatsDisplay(window.showStats);
    director->setAnimationInterval(1.0f / 60.0f);
    applyDesignResolution();

#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID) && (AX_TARGET_PLATFORM != AX_PLATFORM_IOS)
    director->getEventDispatcher()->addCustomEventListener(RenderViewImpl::EVENT_WINDOW_RESIZED, [this](EventCustom*)
    {
        applyDesignResolution();
    });
#endif

    mContext->preloadAtlases();
    hg::launchApplication(*mContext);
    return true;
}

void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();
    if (mContext)
    {
        mContext->audio().pauseAll();
    }
}

void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();
    if (mContext)
    {
        mContext->audio().resumeAll();
    }
}

void AppDelegate::applicationWillQuit()
{
    if (mContext)
    {
        mContext->input().detach();
    }
}

void AppDelegate::applyDesignResolution()
{
    auto* renderView = Director::getInstance()->getRenderView();
    if (!renderView || !mContext)
    {
        return;
    }
    const Vec2 frame = renderView->getFrameSize();
    if (frame.x <= 0.0f || frame.y <= 0.0f)
    {
        return;
    }
    const float designHeight = mContext->config().designHeight;
    renderView->setDesignResolutionSize(designHeight * frame.x / frame.y, designHeight, ResolutionPolicy::FIXED_HEIGHT);
}
