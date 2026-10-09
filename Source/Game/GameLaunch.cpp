#include "Launch.h"

#include "Runtime/App/AppContext.h"
#include "Runtime/App/Log.h"
#include "Runtime/Scenes/PlayScene.h"

namespace hg
{
    void prepareContent(AppContext& aContext)
    {
    }

    WindowSettings windowSettings(const AppConfig& aConfig)
    {
        WindowSettings settings;
        settings.title = aConfig.title;
        settings.size = aConfig.windowSize;
        settings.fullscreen = aConfig.fullscreen;
        settings.resizable = true;
        return settings;
    }

    void launchApplication(AppContext& aContext)
    {
        auto* scene = PlayScene::create(aContext);
        if (!scene->loadScene(aContext.config().startScene))
        {
            Log::error("Start scene could not be loaded");
        }
        ax::Director::getInstance()->runWithScene(scene);
    }
}
