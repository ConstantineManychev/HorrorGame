#include "Core/App/AppConfig.h"

#include "Core/Base/ValueConvert.h"

#include <array>

namespace hg
{
    namespace
    {
        constexpr std::array<std::string_view, 9> kKnownKeys{"format", "version", "title", "start_scene", "window_size", "fullscreen", "design_height", "preload_atlases", "input"};

        bool isKnownKey(std::string_view aKey)
        {
            for (std::string_view known : kKnownKeys)
            {
                if (known == aKey)
                {
                    return true;
                }
            }
            return false;
        }
    }

    std::optional<AppConfig> readAppConfig(const Value& aRoot, Diagnostics& aDiagnostics)
    {
        const ValueObject* root = aRoot.getObject();
        if (!root)
        {
            aDiagnostics.error("", "app config root must be a JSON object");
            return std::nullopt;
        }
        if (root->get("format").asString() != kAppConfigFormat)
        {
            aDiagnostics.warning("format", "expected format '" + std::string(kAppConfigFormat) + "'");
        }

        AppConfig config;
        config.title = root->get("title").asString(config.title);
        config.startScene = root->get("start_scene").asString();
        config.windowSize = readVec2(root->get("window_size"), config.windowSize);
        config.fullscreen = root->get("fullscreen").asBool(false);
        config.designHeight = root->get("design_height").asFloat(config.designHeight);
        config.preloadAtlases = readStringList(root->get("preload_atlases"));

        if (const ValueObject* input = root->get("input").getObject())
        {
            for (const auto& member : *input)
            {
                config.input.push_back({member.key, readStringList(member.value)});
            }
        }
        else
        {
            config.input = defaultInputBindings();
        }

        if (config.startScene.empty())
        {
            aDiagnostics.error("start_scene", "start scene is not specified");
        }
        if (config.designHeight <= 0.0f)
        {
            aDiagnostics.error("design_height", "design height must be positive");
            config.designHeight = 720.0f;
        }

        for (const auto& member : *root)
        {
            if (!isKnownKey(member.key))
            {
                config.extra.set(member.key, member.value);
            }
        }
        return config;
    }

    Value writeAppConfig(const AppConfig& aConfig)
    {
        ValueObject root;
        root.set("format", Value(kAppConfigFormat));
        root.set("version", Value(2));
        root.set("title", Value(aConfig.title));
        root.set("start_scene", Value(aConfig.startScene));
        root.set("window_size", writeVec2(aConfig.windowSize));
        root.set("fullscreen", Value(aConfig.fullscreen));
        root.set("design_height", Value(aConfig.designHeight));
        root.set("preload_atlases", writeStringList(aConfig.preloadAtlases));

        ValueObject input;
        for (const auto& binding : aConfig.input)
        {
            input.set(binding.action, writeStringList(binding.keys));
        }
        root.set("input", Value(std::move(input)));

        for (const auto& member : aConfig.extra)
        {
            root.set(member.key, member.value);
        }
        return Value(std::move(root));
    }

    std::vector<InputBinding> defaultInputBindings()
    {
        return {
            {"move_left", {"KEY_A", "KEY_LEFT_ARROW"}},
            {"move_right", {"KEY_D", "KEY_RIGHT_ARROW"}},
            {"move_up", {"KEY_W", "KEY_UP_ARROW"}},
            {"move_down", {"KEY_S", "KEY_DOWN_ARROW"}},
            {"jump", {"KEY_SPACE"}},
            {"use", {"KEY_E"}},
            {"run", {"KEY_SHIFT"}},
            {"pause", {"KEY_ESCAPE"}},
        };
    }
}
