#include "Core/Scene/Validator.h"

#include "Core/Base/ValueConvert.h"

#include <unordered_map>
#include <unordered_set>

namespace hg
{
    namespace
    {
        class SceneValidator
        {
        public:
            SceneValidator(const SceneDocument* aDocument, const ValidationContext& aContext, Diagnostics& aDiagnostics)
                : mDocument(aDocument)
                , mContext(aContext)
                , mSchema(*aContext.schema)
                , mDiagnostics(aDiagnostics)
            {
            }

            void validateObjects(const std::vector<ObjectDesc>& aObjects, const std::string& aPath)
            {
                for (size_t index = 0; index < aObjects.size(); ++index)
                {
                    validateObject(aObjects[index], aPath + "[" + std::to_string(index) + "]");
                }
            }

            void validateValue(const Value& aValue, const PropertySchema& aProperty, const std::string& aPath)
            {
                if (!valueMatchesProperty(aValue, aProperty))
                {
                    mDiagnostics.error(aPath, "value does not match type " + std::string(propertyTypeName(aProperty.type)));
                    return;
                }

                switch (aProperty.type)
                {
                case PropertyType::Asset:
                    checkAsset(aValue.asString(), aPath);
                    break;
                case PropertyType::ObjectRef:
                    checkObjectRef(aValue, aProperty.refType, aPath);
                    break;
                case PropertyType::SceneRef:
                    if (mContext.sceneExists && !aValue.asString().empty() && !mContext.sceneExists(aValue.asString()))
                    {
                        mDiagnostics.error(aPath, "scene '" + aValue.asString() + "' does not exist");
                    }
                    break;
                case PropertyType::TimelineRef:
                    if (mDocument && !aValue.asString().empty() && !mDocument->findTimeline(aValue.asString()))
                    {
                        mDiagnostics.error(aPath, "timeline '" + aValue.asString() + "' does not exist in this scene");
                    }
                    break;
                case PropertyType::ActionList:
                    validateActions(aValue, aPath);
                    break;
                default:
                    break;
                }
            }

            void validateActions(const Value& aActions, const std::string& aPath)
            {
                const ValueArray& actions = aActions.asArray();
                for (size_t index = 0; index < actions.size(); ++index)
                {
                    validateAction(actions[index].asObject(), aPath + "[" + std::to_string(index) + "]");
                }
            }

            void validateAction(const ValueObject& aAction, const std::string& aPath)
            {
                const std::string id = aAction.get("do").asString();
                const ActionSchema* action = mSchema.findAction(id);
                if (!action)
                {
                    mDiagnostics.error(aPath, "unknown action '" + id + "'");
                    return;
                }
                for (const auto& member : aAction)
                {
                    if (member.key == "do")
                    {
                        continue;
                    }
                    const PropertySchema* argument = nullptr;
                    for (const auto& candidate : action->arguments)
                    {
                        if (candidate.id == member.key)
                        {
                            argument = &candidate;
                            break;
                        }
                    }
                    if (!argument)
                    {
                        mDiagnostics.warning(aPath + "." + member.key, "unknown argument for action '" + id + "'");
                        continue;
                    }
                    validateValue(member.value, *argument, aPath + "." + member.key);
                }
            }

            void validateTimelines(const std::vector<TimelineDesc>& aTimelines)
            {
                std::unordered_set<std::string> ids;
                for (size_t index = 0; index < aTimelines.size(); ++index)
                {
                    const TimelineDesc& timeline = aTimelines[index];
                    const std::string path = "timelines[" + std::to_string(index) + "]";
                    if (!ids.insert(timeline.id).second)
                    {
                        mDiagnostics.error(path, "duplicate timeline id '" + timeline.id + "'");
                    }
                    for (size_t trackIndex = 0; trackIndex < timeline.tracks.size(); ++trackIndex)
                    {
                        validateTrack(timeline, timeline.tracks[trackIndex], path + ".tracks[" + std::to_string(trackIndex) + "]");
                    }
                }
            }

            void collectTypes(const std::vector<ObjectDesc>& aObjects)
            {
                for (const auto& object : aObjects)
                {
                    mTypes[object.uid] = effectiveObjectType(object, mContext.prefabs, mDiagnostics);
                    collectTypes(object.children);
                }
            }

        private:
            void validateObject(const ObjectDesc& aObject, const std::string& aPath)
            {
                const std::string objectPath = aPath + " '" + aObject.name + "'";
                if (aObject.uid == kInvalidObjectId)
                {
                    mDiagnostics.error(objectPath, "object has no uid");
                }
                else if (!mUids.insert(aObject.uid).second)
                {
                    mDiagnostics.error(objectPath, "duplicate uid " + std::to_string(aObject.uid));
                }

                if (!aObject.prefab.empty() && mContext.prefabs && !mContext.prefabs->find(aObject.prefab, mDiagnostics))
                {
                    mDiagnostics.error(objectPath + ".prefab", "prefab '" + aObject.prefab + "' cannot be loaded");
                }

                const std::string type = effectiveObjectType(aObject, mContext.prefabs, mDiagnostics);
                const TypeSchema* typeSchema = mSchema.findType(type);
                if (!typeSchema)
                {
                    if (!type.empty())
                    {
                        mDiagnostics.error(objectPath + ".type", "unknown object type '" + type + "'");
                    }
                }
                else
                {
                    if (!typeSchema->creatable)
                    {
                        mDiagnostics.error(objectPath + ".type", "type '" + type + "' cannot be instantiated");
                    }
                    if (!typeSchema->allowsChildren && !aObject.children.empty())
                    {
                        mDiagnostics.error(objectPath + ".children", "type '" + type + "' cannot have children");
                    }
                    for (const auto& member : aObject.props)
                    {
                        const PropertySchema* property = mSchema.findProperty(type, member.key);
                        if (!property)
                        {
                            mDiagnostics.warning(objectPath + ".props." + member.key, "unknown property for type '" + type + "'");
                            continue;
                        }
                        validateValue(member.value, *property, objectPath + ".props." + member.key);
                    }
                }

                for (size_t index = 0; index < aObject.components.size(); ++index)
                {
                    const ComponentDesc& component = aObject.components[index];
                    const std::string componentPath = objectPath + ".components[" + std::to_string(index) + "]";
                    const ComponentSchema* componentSchema = mSchema.findComponent(component.type);
                    if (!componentSchema)
                    {
                        mDiagnostics.error(componentPath, "unknown component '" + component.type + "'");
                        continue;
                    }
                    for (const auto& member : component.props)
                    {
                        const PropertySchema* property = mSchema.findComponentProperty(component.type, member.key);
                        if (!property)
                        {
                            mDiagnostics.warning(componentPath + "." + member.key, "unknown property for component '" + component.type + "'");
                            continue;
                        }
                        validateValue(member.value, *property, componentPath + "." + member.key);
                    }
                }

                validateObjects(aObject.children, objectPath + ".children");
            }

            void validateTrack(const TimelineDesc& aTimeline, const TrackDesc& aTrack, const std::string& aPath)
            {
                if (aTrack.kind != TrackKind::Event)
                {
                    if (aTrack.target.kind == TargetRef::Kind::None)
                    {
                        mDiagnostics.error(aPath + ".target", "track has no target");
                        return;
                    }
                    if (aTrack.target.isObject() && mDocument && !mDocument->findObject(aTrack.target.uid))
                    {
                        mDiagnostics.error(aPath + ".target", "target object " + std::to_string(aTrack.target.uid) + " does not exist");
                        return;
                    }
                }

                switch (aTrack.kind)
                {
                case TrackKind::Property:
                {
                    const PropertySchema* property = nullptr;
                    if (aTrack.target.isCamera())
                    {
                        property = mSchema.findCameraProperty(aTrack.property);
                    }
                    else
                    {
                        auto type = mTypes.find(aTrack.target.uid);
                        if (type != mTypes.end())
                        {
                            property = mSchema.findProperty(type->second, aTrack.property);
                        }
                    }
                    if (!property)
                    {
                        mDiagnostics.error(aPath + ".property", "property '" + aTrack.property + "' does not exist on the target");
                        return;
                    }
                    if (!property->animatable)
                    {
                        mDiagnostics.error(aPath + ".property", "property '" + aTrack.property + "' is not animatable");
                    }
                    for (size_t index = 0; index < aTrack.keys.size(); ++index)
                    {
                        const Keyframe& key = aTrack.keys[index];
                        const std::string keyPath = aPath + ".keys[" + std::to_string(index) + "]";
                        if (!valueMatchesProperty(key.value, *property))
                        {
                            mDiagnostics.error(keyPath, "key value does not match type " + std::string(propertyTypeName(property->type)));
                        }
                        if (key.time < 0.0f || key.time > aTimeline.duration + 1e-4f)
                        {
                            mDiagnostics.warning(keyPath, "key time is outside of the timeline duration");
                        }
                    }
                    break;
                }
                case TrackKind::Path:
                {
                    auto type = mTypes.find(aTrack.path);
                    if (type == mTypes.end() || type->second != "Path")
                    {
                        mDiagnostics.error(aPath + ".path", "path track must reference a Path object");
                    }
                    if (aTrack.start + aTrack.duration > aTimeline.duration + 1e-4f)
                    {
                        mDiagnostics.warning(aPath, "path clip ends after the timeline duration");
                    }
                    break;
                }
                case TrackKind::Event:
                    for (size_t index = 0; index < aTrack.events.size(); ++index)
                    {
                        validateAction(aTrack.events[index].action, aPath + ".events[" + std::to_string(index) + "].action");
                    }
                    break;
                }
            }

            void checkAsset(const std::string& aPath, const std::string& aDiagnosticPath)
            {
                if (!aPath.empty() && mContext.assetExists && !mContext.assetExists(aPath))
                {
                    mDiagnostics.error(aDiagnosticPath, "asset '" + aPath + "' does not exist");
                }
            }

            void checkObjectRef(const Value& aValue, const std::string& aRefType, const std::string& aPath)
            {
                const ObjectId uid = static_cast<ObjectId>(std::max<int64_t>(0, aValue.asInt()));
                if (uid == kInvalidObjectId || !mDocument)
                {
                    return;
                }
                auto type = mTypes.find(uid);
                if (type == mTypes.end())
                {
                    mDiagnostics.error(aPath, "referenced object " + std::to_string(uid) + " does not exist");
                    return;
                }
                if (!aRefType.empty() && !mSchema.isKindOf(type->second, aRefType))
                {
                    mDiagnostics.error(aPath, "referenced object must be of type '" + aRefType + "'");
                }
            }

            const SceneDocument* mDocument;
            const ValidationContext& mContext;
            const SchemaRegistry& mSchema;
            Diagnostics& mDiagnostics;
            std::unordered_set<ObjectId> mUids;
            std::unordered_map<ObjectId, std::string> mTypes;
        };
    }

    std::string effectiveObjectType(const ObjectDesc& aObject, PrefabLibrary* aPrefabs, Diagnostics& aDiagnostics)
    {
        if (!aObject.type.empty() || aObject.prefab.empty() || !aPrefabs)
        {
            return aObject.type;
        }
        Diagnostics ignored;
        const ObjectDesc* prefab = aPrefabs->find(aObject.prefab, ignored);
        if (!prefab)
        {
            return aObject.type;
        }
        return effectiveObjectType(*prefab, aPrefabs, aDiagnostics);
    }

    void validateScene(const SceneDocument& aDocument, const ValidationContext& aContext, Diagnostics& aDiagnostics)
    {
        SceneValidator validator(&aDocument, aContext, aDiagnostics);
        if (aDocument.id.empty())
        {
            aDiagnostics.error("id", "scene has no id");
        }

        for (const auto& member : aDocument.settings)
        {
            const PropertySchema* setting = aContext.schema->findSetting(member.key);
            if (!setting)
            {
                aDiagnostics.warning("settings." + member.key, "unknown scene setting");
                continue;
            }
            validator.validateValue(member.value, *setting, "settings." + member.key);
        }

        validator.collectTypes(aDocument.objects);
        validator.validateObjects(aDocument.objects, "objects");
        validator.validateTimelines(aDocument.timelines);
    }

    void validatePrefab(const ObjectDesc& aRoot, const ValidationContext& aContext, Diagnostics& aDiagnostics)
    {
        SceneValidator validator(nullptr, aContext, aDiagnostics);
        validator.collectTypes({aRoot});
        validator.validateObjects({aRoot}, "root");
    }

    void validateActionList(const Value& aActions, const SceneDocument* aDocument, const ValidationContext& aContext, Diagnostics& aDiagnostics, const std::string& aPath)
    {
        SceneValidator validator(aDocument, aContext, aDiagnostics);
        if (aDocument)
        {
            validator.collectTypes(aDocument->objects);
        }
        validator.validateActions(aActions, aPath);
    }
}
