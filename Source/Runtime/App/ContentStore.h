#pragma once

#include "Core/App/AppConfig.h"
#include "Core/Base/Diagnostics.h"
#include "Core/Scene/Document.h"
#include "Core/Scene/Prefab.h"

#include <optional>
#include <string>
#include <vector>

namespace hg
{
    class ContentStore
    {
    public:
        ContentStore();

        std::optional<std::string> readText(const std::string& aPath) const;
        bool exists(const std::string& aPath) const;
        std::string fullPath(const std::string& aPath) const;

        void setWritableRoot(std::string aRoot);
        const std::string& writableRoot() const;
        bool canWrite() const;
        bool writeText(const std::string& aPath, const std::string& aText) const;
        bool removeFile(const std::string& aPath) const;

        std::optional<AppConfig> loadAppConfig(Diagnostics& aDiagnostics) const;
        std::optional<SceneDocument> loadScene(const std::string& aSceneId, Diagnostics& aDiagnostics) const;
        bool saveScene(const SceneDocument& aDocument, Diagnostics& aDiagnostics) const;
        bool deleteScene(const std::string& aSceneId) const;
        std::vector<std::string> listScenes() const;
        std::vector<std::string> listFiles(const std::string& aDirectory, const std::vector<std::string>& aExtensions) const;

        PrefabLibrary& prefabs();

    private:
        std::string mWritableRoot;
        PrefabLibrary mPrefabs;
    };
}
