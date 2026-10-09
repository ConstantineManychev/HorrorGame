#include "Launch.h"

#include "Editor/EditorScene.h"
#include "Runtime/App/AppContext.h"
#include "Runtime/App/Log.h"

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
        Log::info("Editor writes content to " + aContext.content().writableRoot());
        ax::Director::getInstance()->runWithScene(EditorScene::create(aContext));
    }
}
