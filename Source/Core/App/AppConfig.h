#pragma once

#include "Core/Base/Diagnostics.h"
#include "Core/Base/Math.h"
#include "Core/Base/Value.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hg
{
    inline constexpr std::string_view kAppConfigPath = "config/app.json";
    inline constexpr std::string_view kAppConfigFormat = "horror-app";

    struct InputBinding
    {
        std::string action;
        std::vector<std::string> keys;

        bool operator==(const InputBinding& aOther) const = default;
    };

    struct AppConfig
    {
        std::string title = "Horror Game";
        std::string startScene;
        Vec2 windowSize{1280.0f, 720.0f};
        bool fullscreen = false;
        float designHeight = 720.0f;
        std::vector<std::string> preloadAtlases;
        std::vector<InputBinding> input;
        ValueObject extra;
    };

    std::optional<AppConfig> readAppConfig(const Value& aRoot, Diagnostics& aDiagnostics);
    Value writeAppConfig(const AppConfig& aConfig);
    std::vector<InputBinding> defaultInputBindings();
}
