#pragma once

#include "Core/App/AppConfig.h"
#include "Core/Base/Math.h"

#include <string>

namespace hg
{
    class AppContext;

    struct WindowSettings
    {
        std::string title;
        Vec2 size{1280.0f, 720.0f};
        bool fullscreen = false;
        bool resizable = false;
        bool showStats = false;
    };

    void prepareContent(AppContext& aContext);
    WindowSettings windowSettings(const AppConfig& aConfig);
    void launchApplication(AppContext& aContext);
}
