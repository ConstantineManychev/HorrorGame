#include "Runtime/App/ContentStore.h"

#include "Core/Base/Json.h"
#include "Core/Scene/SceneSerializer.h"

#include "axmol.h"

#include <algorithm>
#include <set>

namespace hg
{
    namespace
    {
        bool hasExtension(const std::string& aPath, const std::vector<std::string>& aExtensions)
        {
            if (aExtensions.empty())
            {
                return true;
            }
            for (const auto& extension : aExtensions)
            {
                if (aPath.size() >= extension.size() && aPath.compare(aPath.size() - extension.size(), extension.size(), extension) == 0)
                {
                    return true;
                }
            }
            return false;
        }

        std::string normalizeSlashes(std::string aPath)
        {
            std::replace(aPath.begin(), aPath.end(), '\\', '/');
            return aPath;
        }

        std::string trimTrailingSlash(std::string aPath)
        {
            while (!aPath.empty() && (aPath.back() == '/' || aPath.back() == '\\'))
            {
                aPath.pop_back();
            }
            return aPath;
        }
    }

    ContentStore::ContentStore()
        : mPrefabs([this](const std::string& aPath)
        {
            return readText(aPath);
        })
    {
    }

    std::optional<std::string> ContentStore::readText(const std::string& aPath) const
    {
        auto* fileUtils = ax::FileUtils::getInstance();
        if (!fileUtils->isFileExist(aPath))
        {
            return std::nullopt;
        }
        return fileUtils->getStringFromFile(aPath);
    }

    bool ContentStore::exists(const std::string& aPath) const
    {
        return !aPath.empty() && ax::FileUtils::getInstance()->isFileExist(aPath);
    }

    std::string ContentStore::fullPath(const std::string& aPath) const
    {
        return ax::FileUtils::getInstance()->fullPathForFilename(aPath);
    }

    void ContentStore::setWritableRoot(std::string aRoot)
    {
        mWritableRoot = trimTrailingSlash(std::move(aRoot));
        if (!mWritableRoot.empty())
        {
            ax::FileUtils::getInstance()->addSearchPath(mWritableRoot, true);
        }
    }

    const std::string& ContentStore::writableRoot() const
    {
        return mWritableRoot;
    }

    bool ContentStore::canWrite() const
    {
        return !mWritableRoot.empty();
    }

    bool ContentStore::writeText(const std::string& aPath, const std::string& aText) const
    {
        if (!canWrite())
        {
            return false;
        }
        auto* fileUtils = ax::FileUtils::getInstance();
        const std::string fullPath = mWritableRoot + "/" + aPath;
        const size_t slash = fullPath.find_last_of('/');
        if (slash != std::string::npos)
        {
            fileUtils->createDirectories(fullPath.substr(0, slash));
        }
        const bool written = fileUtils->writeStringToFile(aText, fullPath);
        fileUtils->purgeCachedEntries();
        return written;
    }

    bool ContentStore::removeFile(const std::string& aPath) const
    {
        if (!canWrite())
        {
            return false;
        }
        auto* fileUtils = ax::FileUtils::getInstance();
        const bool removed = fileUtils->removeFile(mWritableRoot + "/" + aPath);
        fileUtils->purgeCachedEntries();
        return removed;
    }

    std::optional<AppConfig> ContentStore::loadAppConfig(Diagnostics& aDiagnostics) const
    {
        aDiagnostics.setSource(std::string(kAppConfigPath));
        auto text = readText(std::string(kAppConfigPath));
        if (!text)
        {
            aDiagnostics.error("", "app config not found");
            return std::nullopt;
        }
        auto parsed = parseJson(*text);
        if (!parsed)
        {
            aDiagnostics.error("", parsed.error());
            return std::nullopt;
        }
        return readAppConfig(parsed.value(), aDiagnostics);
    }

    std::optional<SceneDocument> ContentStore::loadScene(const std::string& aSceneId, Diagnostics& aDiagnostics) const
    {
        const std::string path = sceneFilePath(aSceneId);
        aDiagnostics.setSource(path);
        auto text = readText(path);
        if (!text)
        {
            aDiagnostics.error("", "scene file not found");
            return std::nullopt;
        }
        auto parsed = parseJson(*text);
        if (!parsed)
        {
            aDiagnostics.error("", parsed.error());
            return std::nullopt;
        }
        return readScene(parsed.value(), aDiagnostics);
    }

    bool ContentStore::saveScene(const SceneDocument& aDocument, Diagnostics& aDiagnostics) const
    {
        const std::string path = sceneFilePath(aDocument.id);
        if (!writeText(path, writeJson(writeScene(aDocument))))
        {
            aDiagnostics.setSource(path);
            aDiagnostics.error("", canWrite() ? "cannot write scene file" : "content is read-only in this build");
            return false;
        }
        return true;
    }

    bool ContentStore::deleteScene(const std::string& aSceneId) const
    {
        return removeFile(sceneFilePath(aSceneId));
    }

    std::vector<std::string> ContentStore::listScenes() const
    {
        std::vector<std::string> ids;
        for (const auto& file : listFiles(std::string(kScenesDirectory), {".json"}))
        {
            const size_t slash = file.find_last_of('/');
            const std::string name = slash == std::string::npos ? file : file.substr(slash + 1);
            ids.push_back(name.substr(0, name.size() - 5));
        }
        return ids;
    }

    std::vector<std::string> ContentStore::listFiles(const std::string& aDirectory, const std::vector<std::string>& aExtensions) const
    {
        auto* fileUtils = ax::FileUtils::getInstance();
        std::set<std::string> unique;
        std::vector<std::string> roots;
        if (canWrite())
        {
            roots.push_back(mWritableRoot);
        }
        roots.push_back(trimTrailingSlash(fileUtils->getDefaultResourceRootPath()));

        for (const auto& root : roots)
        {
            const std::string prefix = root.empty() ? std::string() : normalizeSlashes(root) + "/";
            const std::string directory = prefix + aDirectory;
            if (!fileUtils->isDirectoryExist(directory))
            {
                continue;
            }
            std::vector<std::string> files;
            fileUtils->listFilesRecursively(directory, &files);
            for (const auto& rawFile : files)
            {
                const std::string file = normalizeSlashes(rawFile);
                if (file.empty() || file.back() == '/' || !hasExtension(file, aExtensions))
                {
                    continue;
                }
                if (file.compare(0, prefix.size(), prefix) == 0)
                {
                    unique.insert(file.substr(prefix.size()));
                }
            }
        }
        return {unique.begin(), unique.end()};
    }

    PrefabLibrary& ContentStore::prefabs()
    {
        return mPrefabs;
    }
}
