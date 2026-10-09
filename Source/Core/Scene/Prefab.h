#pragma once

#include "Core/Base/Diagnostics.h"
#include "Core/Scene/Document.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace hg
{
    using TextFileReader = std::function<std::optional<std::string>(const std::string& aPath)>;

    class PrefabLibrary
    {
    public:
        explicit PrefabLibrary(TextFileReader aReader);

        const ObjectDesc* find(const std::string& aPath, Diagnostics& aDiagnostics);
        void invalidate();

    private:
        TextFileReader mReader;
        std::unordered_map<std::string, std::optional<ObjectDesc>> mCache;
    };

    ObjectDesc mergePrefabInstance(const ObjectDesc& aInstance, const ObjectDesc& aPrefabRoot);
    ObjectDesc expandPrefabs(const ObjectDesc& aObject, PrefabLibrary& aLibrary, Diagnostics& aDiagnostics, int aDepth = 0);
    std::vector<ObjectDesc> expandPrefabs(const std::vector<ObjectDesc>& aObjects, PrefabLibrary& aLibrary, Diagnostics& aDiagnostics);
}
