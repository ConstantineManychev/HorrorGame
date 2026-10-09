#pragma once

#include "Core/Base/Diagnostics.h"
#include "Core/Base/Value.h"
#include "Core/Scene/Document.h"

#include <optional>
#include <string>
#include <string_view>

namespace hg
{
    inline constexpr std::string_view kSceneFormat = "horror-scene";
    inline constexpr std::string_view kPrefabFormat = "horror-prefab";
    inline constexpr std::string_view kScenesDirectory = "scenes";
    inline constexpr std::string_view kPrefabsDirectory = "prefabs";

    std::optional<SceneDocument> readScene(const Value& aRoot, Diagnostics& aDiagnostics);
    Value writeScene(const SceneDocument& aDocument);

    ObjectDesc readObject(const Value& aValue, Diagnostics& aDiagnostics, const std::string& aPath);
    Value writeObject(const ObjectDesc& aObject);

    std::optional<ObjectDesc> readPrefab(const Value& aRoot, Diagnostics& aDiagnostics);
    Value writePrefab(const ObjectDesc& aRoot);

    std::string sceneFilePath(std::string_view aSceneId);
    SceneDocument createEmptyScene(std::string aId, SceneKind aKind);
}
