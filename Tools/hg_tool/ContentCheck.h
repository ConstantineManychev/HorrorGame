#pragma once

#include "Core/Base/Diagnostics.h"

#include <filesystem>
#include <optional>
#include <string>

namespace hg::tool
{
    struct ContentReport
    {
        Diagnostics diagnostics;
        size_t scenes = 0;
        size_t prefabs = 0;
    };

    std::optional<std::string> readTextFile(const std::filesystem::path& aPath);
    bool writeTextFile(const std::filesystem::path& aPath, const std::string& aText);
    ContentReport checkContent(const std::filesystem::path& aContentRoot);
}
