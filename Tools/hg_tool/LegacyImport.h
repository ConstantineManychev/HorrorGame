#pragma once

#include "Core/Base/Diagnostics.h"

#include <filesystem>
#include <string>
#include <vector>

namespace hg::tool
{
    struct LegacyImportResult
    {
        Diagnostics diagnostics;
        std::vector<std::string> writtenFiles;
    };

    LegacyImportResult importLegacyContent(const std::filesystem::path& aLegacyRoot, const std::filesystem::path& aContentRoot);
}
