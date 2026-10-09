#include "Core/Scene/Prefab.h"

#include "Core/Base/Json.h"
#include "Core/Scene/SceneSerializer.h"

namespace hg
{
    namespace
    {
        constexpr int kMaxPrefabDepth = 8;

        void remapChildIds(std::vector<ObjectDesc>& aChildren, ObjectId aInstanceId)
        {
            for (auto& child : aChildren)
            {
                child.uid = combinePrefabIds(aInstanceId, child.uid);
                remapChildIds(child.children, aInstanceId);
            }
        }
    }

    PrefabLibrary::PrefabLibrary(TextFileReader aReader)
        : mReader(std::move(aReader))
    {
    }

    const ObjectDesc* PrefabLibrary::find(const std::string& aPath, Diagnostics& aDiagnostics)
    {
        auto cached = mCache.find(aPath);
        if (cached != mCache.end())
        {
            return cached->second ? &*cached->second : nullptr;
        }

        std::optional<ObjectDesc> prefab;
        Diagnostics local;
        local.setSource(aPath);
        if (auto text = mReader ? mReader(aPath) : std::nullopt)
        {
            auto parsed = parseJson(*text);
            if (parsed)
            {
                prefab = readPrefab(parsed.value(), local);
            }
            else
            {
                local.error("", parsed.error());
            }
        }
        else
        {
            local.error("", "prefab file not found");
        }
        aDiagnostics.append(local);

        auto inserted = mCache.emplace(aPath, std::move(prefab)).first;
        return inserted->second ? &*inserted->second : nullptr;
    }

    void PrefabLibrary::invalidate()
    {
        mCache.clear();
    }

    ObjectDesc mergePrefabInstance(const ObjectDesc& aInstance, const ObjectDesc& aPrefabRoot)
    {
        ObjectDesc result = aPrefabRoot;
        result.uid = aInstance.uid;
        result.name = aInstance.name;
        result.prefab = aInstance.prefab;
        remapChildIds(result.children, aInstance.uid);

        for (const auto& member : aInstance.props)
        {
            result.props.set(member.key, member.value);
        }

        for (const auto& component : aInstance.components)
        {
            if (ComponentDesc* existing = result.findComponent(component.type))
            {
                for (const auto& member : component.props)
                {
                    existing->props.set(member.key, member.value);
                }
            }
            else
            {
                result.components.push_back(component);
            }
        }

        result.children.insert(result.children.end(), aInstance.children.begin(), aInstance.children.end());
        return result;
    }

    ObjectDesc expandPrefabs(const ObjectDesc& aObject, PrefabLibrary& aLibrary, Diagnostics& aDiagnostics, int aDepth)
    {
        ObjectDesc result = aObject;
        if (!aObject.prefab.empty())
        {
            if (aDepth >= kMaxPrefabDepth)
            {
                aDiagnostics.error(aObject.name, "prefab nesting is too deep at '" + aObject.prefab + "'");
            }
            else if (const ObjectDesc* prefab = aLibrary.find(aObject.prefab, aDiagnostics))
            {
                ObjectDesc expandedPrefab = expandPrefabs(*prefab, aLibrary, aDiagnostics, aDepth + 1);
                result = mergePrefabInstance(aObject, expandedPrefab);
            }
            result.prefab.clear();
        }

        for (auto& child : result.children)
        {
            child = expandPrefabs(child, aLibrary, aDiagnostics, aDepth);
        }
        return result;
    }

    std::vector<ObjectDesc> expandPrefabs(const std::vector<ObjectDesc>& aObjects, PrefabLibrary& aLibrary, Diagnostics& aDiagnostics)
    {
        std::vector<ObjectDesc> result;
        result.reserve(aObjects.size());
        for (const auto& object : aObjects)
        {
            result.push_back(expandPrefabs(object, aLibrary, aDiagnostics));
        }
        return result;
    }
}
