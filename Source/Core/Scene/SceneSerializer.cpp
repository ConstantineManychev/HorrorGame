#include "Core/Scene/SceneSerializer.h"

#include <array>
#include <functional>

namespace hg
{
    namespace
    {
        using Migration = std::function<void(ValueObject&, Diagnostics&)>;

        const std::vector<Migration>& migrationsFromVersion2()
        {
            static const std::vector<Migration> migrations;
            return migrations;
        }

        constexpr std::array<std::string_view, 9> kKnownSceneKeys{"format", "version", "id", "kind", "next_uid", "settings", "objects", "timelines", "$schema"};

        bool isKnownSceneKey(std::string_view aKey)
        {
            for (std::string_view known : kKnownSceneKeys)
            {
                if (known == aKey)
                {
                    return true;
                }
            }
            return false;
        }

        ComponentDesc readComponent(const Value& aValue, Diagnostics& aDiagnostics, const std::string& aPath)
        {
            ComponentDesc component;
            const ValueObject& object = aValue.asObject();
            component.type = object.get("type").asString();
            component.props = object.get("props").asObject();
            if (component.type.empty())
            {
                aDiagnostics.error(aPath, "component has no type");
            }
            return component;
        }

        Value writeComponent(const ComponentDesc& aComponent)
        {
            ValueObject object;
            object.set("type", Value(aComponent.type));
            if (!aComponent.props.empty())
            {
                object.set("props", Value(aComponent.props));
            }
            return Value(std::move(object));
        }
    }

    ObjectDesc readObject(const Value& aValue, Diagnostics& aDiagnostics, const std::string& aPath)
    {
        ObjectDesc desc;
        const ValueObject* object = aValue.getObject();
        if (!object)
        {
            aDiagnostics.error(aPath, "object entry must be a JSON object");
            return desc;
        }

        desc.uid = static_cast<ObjectId>(std::max<int64_t>(0, object->get("uid").asInt()));
        desc.name = object->get("name").asString();
        desc.type = object->get("type").asString();
        desc.prefab = object->get("prefab").asString();
        desc.props = object->get("props").asObject();

        if (desc.type.empty() && desc.prefab.empty())
        {
            aDiagnostics.error(aPath, "object '" + desc.name + "' has neither type nor prefab");
        }

        const ValueArray& components = object->get("components").asArray();
        for (size_t index = 0; index < components.size(); ++index)
        {
            desc.components.push_back(readComponent(components[index], aDiagnostics, aPath + ".components[" + std::to_string(index) + "]"));
        }

        const ValueArray& children = object->get("children").asArray();
        for (size_t index = 0; index < children.size(); ++index)
        {
            desc.children.push_back(readObject(children[index], aDiagnostics, aPath + ".children[" + std::to_string(index) + "]"));
        }
        return desc;
    }

    Value writeObject(const ObjectDesc& aObject)
    {
        ValueObject object;
        object.set("uid", Value(aObject.uid));
        object.set("name", Value(aObject.name));
        if (!aObject.type.empty())
        {
            object.set("type", Value(aObject.type));
        }
        if (!aObject.prefab.empty())
        {
            object.set("prefab", Value(aObject.prefab));
        }
        if (!aObject.props.empty())
        {
            object.set("props", Value(aObject.props));
        }
        if (!aObject.components.empty())
        {
            ValueArray components;
            for (const auto& component : aObject.components)
            {
                components.push_back(writeComponent(component));
            }
            object.set("components", Value(std::move(components)));
        }
        if (!aObject.children.empty())
        {
            ValueArray children;
            for (const auto& child : aObject.children)
            {
                children.push_back(writeObject(child));
            }
            object.set("children", Value(std::move(children)));
        }
        return Value(std::move(object));
    }

    std::optional<SceneDocument> readScene(const Value& aRoot, Diagnostics& aDiagnostics)
    {
        const ValueObject* rootObject = aRoot.getObject();
        if (!rootObject)
        {
            aDiagnostics.error("", "scene root must be a JSON object");
            return std::nullopt;
        }

        ValueObject root = *rootObject;
        const std::string format = root.get("format").asString();
        if (format != kSceneFormat)
        {
            aDiagnostics.warning("format", "expected format '" + std::string(kSceneFormat) + "', got '" + format + "'");
        }

        if (!root.contains("version"))
        {
            aDiagnostics.error("version", "scene has no version, legacy files must be converted with 'hg_tool import-legacy'");
            return std::nullopt;
        }

        int version = static_cast<int>(root.get("version").asInt());
        if (version > SceneDocument::kCurrentVersion)
        {
            aDiagnostics.error("version", "scene version " + std::to_string(version) + " is newer than supported " + std::to_string(SceneDocument::kCurrentVersion));
            return std::nullopt;
        }
        if (version < 2)
        {
            aDiagnostics.error("version", "scene version " + std::to_string(version) + " is not supported, convert it with 'hg_tool import-legacy'");
            return std::nullopt;
        }

        const auto& migrations = migrationsFromVersion2();
        while (version < SceneDocument::kCurrentVersion)
        {
            const size_t index = static_cast<size_t>(version - 2);
            if (index >= migrations.size())
            {
                aDiagnostics.error("version", "missing migration from version " + std::to_string(version));
                return std::nullopt;
            }
            migrations[index](root, aDiagnostics);
            ++version;
        }

        SceneDocument document;
        document.version = SceneDocument::kCurrentVersion;
        document.id = root.get("id").asString();
        if (document.id.empty())
        {
            aDiagnostics.error("id", "scene has no id");
        }

        const std::string kindName = root.get("kind").asString("location");
        if (!parseSceneKind(kindName, document.kind))
        {
            aDiagnostics.error("kind", "unknown scene kind '" + kindName + "'");
        }

        document.nextUid = static_cast<ObjectId>(std::max<int64_t>(1, root.get("next_uid").asInt(1)));
        document.settings = root.get("settings").asObject();

        const ValueArray& objects = root.get("objects").asArray();
        for (size_t index = 0; index < objects.size(); ++index)
        {
            document.objects.push_back(readObject(objects[index], aDiagnostics, "objects[" + std::to_string(index) + "]"));
        }

        const ValueArray& timelines = root.get("timelines").asArray();
        for (size_t index = 0; index < timelines.size(); ++index)
        {
            document.timelines.push_back(readTimeline(timelines[index], aDiagnostics, "timelines[" + std::to_string(index) + "]"));
        }

        for (const auto& member : root)
        {
            if (!isKnownSceneKey(member.key))
            {
                document.extra.set(member.key, member.value);
            }
        }

        document.normalizeUids();
        return document;
    }

    Value writeScene(const SceneDocument& aDocument)
    {
        ValueObject root;
        root.set("format", Value(kSceneFormat));
        root.set("version", Value(SceneDocument::kCurrentVersion));
        root.set("id", Value(aDocument.id));
        root.set("kind", Value(sceneKindName(aDocument.kind)));
        root.set("next_uid", Value(std::max(aDocument.nextUid, aDocument.computeMaxUid() + 1)));
        root.set("settings", Value(aDocument.settings));

        ValueArray objects;
        for (const auto& object : aDocument.objects)
        {
            objects.push_back(writeObject(object));
        }
        root.set("objects", Value(std::move(objects)));

        ValueArray timelines;
        for (const auto& timeline : aDocument.timelines)
        {
            timelines.push_back(writeTimeline(timeline));
        }
        root.set("timelines", Value(std::move(timelines)));

        for (const auto& member : aDocument.extra)
        {
            root.set(member.key, member.value);
        }
        return Value(std::move(root));
    }

    std::optional<ObjectDesc> readPrefab(const Value& aRoot, Diagnostics& aDiagnostics)
    {
        const ValueObject* root = aRoot.getObject();
        if (!root)
        {
            aDiagnostics.error("", "prefab root must be a JSON object");
            return std::nullopt;
        }
        if (root->get("format").asString() != kPrefabFormat)
        {
            aDiagnostics.warning("format", "expected format '" + std::string(kPrefabFormat) + "'");
        }
        const int version = static_cast<int>(root->get("version").asInt(0));
        if (version != SceneDocument::kCurrentVersion)
        {
            aDiagnostics.error("version", "unsupported prefab version " + std::to_string(version));
            return std::nullopt;
        }
        const Value* object = root->find("root");
        if (!object || !object->isObject())
        {
            aDiagnostics.error("root", "prefab has no root object");
            return std::nullopt;
        }
        return readObject(*object, aDiagnostics, "root");
    }

    Value writePrefab(const ObjectDesc& aRoot)
    {
        ValueObject root;
        root.set("format", Value(kPrefabFormat));
        root.set("version", Value(SceneDocument::kCurrentVersion));
        root.set("root", writeObject(aRoot));
        return Value(std::move(root));
    }

    std::string sceneFilePath(std::string_view aSceneId)
    {
        return std::string(kScenesDirectory) + "/" + std::string(aSceneId) + ".json";
    }

    SceneDocument createEmptyScene(std::string aId, SceneKind aKind)
    {
        SceneDocument document;
        document.id = std::move(aId);
        document.kind = aKind;
        for (const auto& setting : builtinSchemas().sceneSettings())
        {
            document.settings.set(setting.id, setting.defaultValue);
        }
        if (aKind == SceneKind::Location)
        {
            document.settings.set("control_mode", Value("side_scroll"));
        }
        return document;
    }
}
