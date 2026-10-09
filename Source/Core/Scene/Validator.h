#pragma once

#include "Core/Base/Diagnostics.h"
#include "Core/Scene/Document.h"
#include "Core/Scene/Prefab.h"
#include "Core/Scene/Schema.h"

#include <functional>
#include <string>

namespace hg
{
    struct ValidationContext
    {
        const SchemaRegistry* schema = &builtinSchemas();
        std::function<bool(const std::string&)> assetExists;
        std::function<bool(const std::string&)> sceneExists;
        PrefabLibrary* prefabs = nullptr;
    };

    void validateScene(const SceneDocument& aDocument, const ValidationContext& aContext, Diagnostics& aDiagnostics);
    void validatePrefab(const ObjectDesc& aRoot, const ValidationContext& aContext, Diagnostics& aDiagnostics);
    void validateActionList(const Value& aActions, const SceneDocument* aDocument, const ValidationContext& aContext, Diagnostics& aDiagnostics, const std::string& aPath);
    std::string effectiveObjectType(const ObjectDesc& aObject, PrefabLibrary* aPrefabs, Diagnostics& aDiagnostics);
}
