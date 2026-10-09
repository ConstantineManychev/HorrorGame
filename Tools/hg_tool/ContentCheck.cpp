#include "ContentCheck.h"

#include "Core/App/AppConfig.h"
#include "Core/Base/Json.h"
#include "Core/Scene/Prefab.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Scene/Validator.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>

namespace hg::tool
{
    namespace
    {
        std::vector<std::filesystem::path> listJsonFiles(const std::filesystem::path& aDirectory)
        {
            std::vector<std::filesystem::path> files;
            std::error_code error;
            if (!std::filesystem::is_directory(aDirectory, error))
            {
                return files;
            }
            for (const auto& entry : std::filesystem::directory_iterator(aDirectory, error))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    files.push_back(entry.path());
                }
            }
            std::sort(files.begin(), files.end());
            return files;
        }

        std::optional<Value> loadJson(const std::filesystem::path& aPath, Diagnostics& aDiagnostics)
        {
            auto text = readTextFile(aPath);
            if (!text)
            {
                aDiagnostics.error("", "cannot read file");
                return std::nullopt;
            }
            auto parsed = parseJson(*text);
            if (!parsed)
            {
                aDiagnostics.error("", parsed.error());
                return std::nullopt;
            }
            return parsed.take();
        }
    }

    std::optional<std::string> readTextFile(const std::filesystem::path& aPath)
    {
        std::ifstream stream(aPath, std::ios::binary);
        if (!stream)
        {
            return std::nullopt;
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        return buffer.str();
    }

    bool writeTextFile(const std::filesystem::path& aPath, const std::string& aText)
    {
        std::error_code error;
        if (aPath.has_parent_path())
        {
            std::filesystem::create_directories(aPath.parent_path(), error);
        }
        std::ofstream stream(aPath, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return false;
        }
        stream << aText;
        return static_cast<bool>(stream);
    }

    ContentReport checkContent(const std::filesystem::path& aContentRoot)
    {
        ContentReport report;
        Diagnostics& diagnostics = report.diagnostics;

        auto assetExists = [aContentRoot](const std::string& aPath)
        {
            std::error_code error;
            return std::filesystem::is_regular_file(aContentRoot / aPath, error);
        };

        std::set<std::string> sceneIds;
        for (const auto& file : listJsonFiles(aContentRoot / kScenesDirectory))
        {
            sceneIds.insert(file.stem().string());
        }
        auto sceneExists = [&sceneIds](const std::string& aId)
        {
            return sceneIds.count(aId) > 0;
        };

        diagnostics.setSource(std::string(kAppConfigPath));
        if (auto root = loadJson(aContentRoot / kAppConfigPath, diagnostics))
        {
            if (auto config = readAppConfig(*root, diagnostics))
            {
                if (!config->startScene.empty() && !sceneExists(config->startScene))
                {
                    diagnostics.error("start_scene", "scene '" + config->startScene + "' does not exist");
                }
                for (const auto& atlas : config->preloadAtlases)
                {
                    if (!assetExists(atlas))
                    {
                        diagnostics.error("preload_atlases", "atlas '" + atlas + "' does not exist");
                    }
                }
            }
        }

        PrefabLibrary prefabs([aContentRoot](const std::string& aPath)
        {
            return readTextFile(aContentRoot / aPath);
        });

        ValidationContext context;
        context.assetExists = assetExists;
        context.sceneExists = sceneExists;
        context.prefabs = &prefabs;

        for (const auto& file : listJsonFiles(aContentRoot / kPrefabsDirectory))
        {
            const std::string relative = std::filesystem::relative(file, aContentRoot).generic_string();
            diagnostics.setSource(relative);
            ++report.prefabs;
            if (auto root = loadJson(file, diagnostics))
            {
                if (auto prefab = readPrefab(*root, diagnostics))
                {
                    validatePrefab(*prefab, context, diagnostics);
                }
            }
        }

        for (const auto& file : listJsonFiles(aContentRoot / kScenesDirectory))
        {
            const std::string relative = std::filesystem::relative(file, aContentRoot).generic_string();
            diagnostics.setSource(relative);
            ++report.scenes;
            if (auto root = loadJson(file, diagnostics))
            {
                if (auto document = readScene(*root, diagnostics))
                {
                    if (document->id != file.stem().string())
                    {
                        diagnostics.error("id", "scene id '" + document->id + "' does not match file name");
                    }
                    validateScene(*document, context, diagnostics);
                }
            }
        }
        return report;
    }
}
