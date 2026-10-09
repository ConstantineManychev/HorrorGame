#include "Launch.h"

#include "Runtime/App/AppContext.h"
#include "Runtime/Scenes/PlayScene.h"

namespace hg
{
    void prepareContent(AppContext& aContext)
    {
        aContext.content().setWritableRoot(HG_CONTENT_SOURCE_DIR);
    }

    WindowSettings windowSettings(const AppConfig& aConfig)
    {
        WindowSettings settings;
        settings.title = aConfig.title + " Editor";
        settings.size = {1600.0f, 900.0f};
        settings.resizable = true;
        return settings;
    }

    void launchApplication(AppContext& aContext)
    {
        auto* scene = PlayScene::create(aContext);
        scene->loadScene(aContext.config().startScene);
        ax::Director::getInstance()->runWithScene(scene);
    }
}
