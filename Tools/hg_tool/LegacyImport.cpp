#include "LegacyImport.h"

#include "ContentCheck.h"

#include "Core/App/AppConfig.h"
#include "Core/Base/Json.h"
#include "Core/Base/ValueConvert.h"
#include "Core/Scene/SceneSerializer.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <set>

namespace hg::tool
{
    namespace
    {
        const std::map<std::string, std::string> kLegacyInputNames{
            {"MoveLeft", "move_left"},
            {"MoveRight", "move_right"},
            {"MoveUp", "move_up"},
            {"MoveDown", "move_down"},
            {"Use", "use"},
            {"Jump", "jump"},
            {"Run", "run"},
        };

        std::string toLower(std::string aText)
        {
            std::transform(aText.begin(), aText.end(), aText.begin(), [](unsigned char aCharacter)
            {
                return static_cast<char>(std::tolower(aCharacter));
            });
            return aText;
        }

        std::string sceneIdFromViewId(std::string aViewId)
        {
            const std::string suffix = "_view";
            if (aViewId.size() > suffix.size() && aViewId.compare(aViewId.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                aViewId.erase(aViewId.size() - suffix.size());
            }
            return toLower(aViewId);
        }

        Vec2 readLegacySize(const Value& aValue, const Vec2& aFallback)
        {
            if (const ValueObject* object = aValue.getObject())
            {
                return {object->get("width").asFloat(aFallback.x), object->get("height").asFloat(aFallback.y)};
            }
            if (aValue.isString())
            {
                std::smatch match;
                const std::string text = aValue.asString();
                static const std::regex pattern(R"(([-0-9.]+)\s*,\s*([-0-9.]+))");
                if (std::regex_search(text, match, pattern))
                {
                    return {std::stof(match[1].str()), std::stof(match[2].str())};
                }
            }
            return aFallback;
        }

        std::string stemOf(const std::string& aPath)
        {
            return std::filesystem::path(aPath).stem().string();
        }

        class LegacyImporter
        {
        public:
            LegacyImporter(std::filesystem::path aLegacyRoot, std::filesystem::path aContentRoot, LegacyImportResult& aResult)
                : mLegacyRoot(std::move(aLegacyRoot))
                , mContentRoot(std::move(aContentRoot))
                , mResult(aResult)
                , mDiagnostics(aResult.diagnostics)
            {
            }

            void run()
            {
                indexContentFiles();
                const ValueObject mainConfig = loadLegacy("configs/main_config.json").asObject();
                const ValueArray views = loadLegacy("configs/views/views_list.json").asArray();

                std::string startScene;
                for (const auto& viewPath : views)
                {
                    mDiagnostics.setSource(viewPath.asString());
                    const ValueObject view = loadLegacy(viewPath.asString()).asObject();
                    if (view.empty())
                    {
                        continue;
                    }
                    SceneDocument document = importView(view);
                    if (startScene.empty())
                    {
                        startScene = document.id;
                    }
                    write(sceneFilePath(document.id), writeScene(document));
                }

                mDiagnostics.setSource("configs/main_config.json");
                write(std::string(kAppConfigPath), writeAppConfig(importMainConfig(mainConfig, startScene)));
            }

        private:
            void indexContentFiles()
            {
                std::error_code error;
                for (const auto& entry : std::filesystem::recursive_directory_iterator(mContentRoot, error))
                {
                    if (!entry.is_regular_file())
                    {
                        continue;
                    }
                    const std::string relative = std::filesystem::relative(entry.path(), mContentRoot).generic_string();
                    mContentFiles.push_back(relative);
                    if (entry.path().extension() == ".plist")
                    {
                        indexAtlasFrames(entry.path());
                    }
                }
                std::sort(mContentFiles.begin(), mContentFiles.end());
            }

            void indexAtlasFrames(const std::filesystem::path& aPlist)
            {
                auto text = readTextFile(aPlist);
                if (!text)
                {
                    return;
                }
                static const std::regex keyPattern(R"(<key>([^<]+\.png)</key>)");
                for (std::sregex_iterator it(text->begin(), text->end(), keyPattern), end; it != end; ++it)
                {
                    mAtlasFrames.insert((*it)[1].str());
                }
            }

            Value loadLegacy(const std::string& aRelativePath)
            {
                auto text = readTextFile(mLegacyRoot / aRelativePath);
                if (!text)
                {
                    mDiagnostics.error(aRelativePath, "legacy file not found");
                    return Value();
                }
                auto parsed = parseJson(*text);
                if (!parsed)
                {
                    mDiagnostics.error(aRelativePath, parsed.error());
                    return Value();
                }
                return parsed.take();
            }

            void write(const std::string& aRelativePath, const Value& aValue)
            {
                if (writeTextFile(mContentRoot / aRelativePath, writeJson(aValue)))
                {
                    mResult.writtenFiles.push_back(aRelativePath);
                }
                else
                {
                    mDiagnostics.error(aRelativePath, "cannot write file");
                }
            }

            AppConfig importMainConfig(const ValueObject& aMainConfig, const std::string& aStartScene)
            {
                AppConfig config;
                config.startScene = aStartScene;
                const ValueObject& startup = aMainConfig.get("startup_settings").asObject();
                config.fullscreen = startup.get("is_full_screen").asBool(false);
                config.windowSize = readLegacySize(startup.get("screen_size"), config.windowSize);
                config.designHeight = config.windowSize.y;

                for (const auto& atlas : readStringList(startup.get("preload_atlases")))
                {
                    if (std::binary_search(mContentFiles.begin(), mContentFiles.end(), atlas))
                    {
                        config.preloadAtlases.push_back(atlas);
                    }
                    else
                    {
                        mDiagnostics.warning("preload_atlases", "dropped missing atlas '" + atlas + "'");
                    }
                }

                config.input = defaultInputBindings();
                for (const auto& member : startup.get("input_bindings").asObject())
                {
                    auto name = kLegacyInputNames.find(member.key);
                    if (name == kLegacyInputNames.end())
                    {
                        mDiagnostics.warning("input_bindings." + member.key, "action is not supported and was dropped");
                        continue;
                    }
                    auto binding = std::find_if(config.input.begin(), config.input.end(), [&name](const InputBinding& aBinding)
                    {
                        return aBinding.action == name->second;
                    });
                    std::vector<std::string> keys = member.value.isString() ? std::vector<std::string>{member.value.asString()} : readStringList(member.value);
                    if (binding != config.input.end())
                    {
                        binding->keys = keys;
                    }
                    else
                    {
                        config.input.push_back({name->second, keys});
                    }
                }
                return config;
            }

            SceneDocument importView(const ValueObject& aView)
            {
                const ValueObject& params = aView.get("params").asObject();
                const SceneKind kind = aView.get("type").asString() == "Location" ? SceneKind::Location : SceneKind::Screen;
                SceneDocument document = createEmptyScene(sceneIdFromViewId(aView.get("id").asString()), kind);

                const Vec2 worldSize = readLegacySize(params.get("content_size"), {3840.0f, 2160.0f});
                document.settings.set("world_size", writeVec2(worldSize));
                document.settings.set("view_height", Value(worldSize.y));

                for (const auto& member : aView.get("children").asObject())
                {
                    document.objects.push_back(importNode(member.key, member.value.asObject(), document, worldSize, "children." + member.key));
                }

                const ValueObject& actions = aView.get("actions").asObject();
                if (const Value* onCreate = actions.find("onCreate"))
                {
                    document.settings.set("on_start", Value(importActions(onCreate->asArray(), document, "actions.onCreate")));
                }
                return document;
            }

            ObjectDesc importNode(const std::string& aName, ValueObject aNode, SceneDocument& aDocument, const Vec2& aParentSize, const std::string& aPath)
            {
                if (const Value* prefabPath = aNode.find("prefab"))
                {
                    ValueObject merged = loadLegacy(prefabPath->asString()).asObject();
                    for (const auto& member : aNode)
                    {
                        if (member.key == "prefab")
                        {
                            continue;
                        }
                        if (member.key == "params")
                        {
                            ValueObject mergedParams = merged.get("params").asObject();
                            for (const auto& param : member.value.asObject())
                            {
                                mergedParams.set(param.key, param.value);
                            }
                            merged.set("params", Value(std::move(mergedParams)));
                        }
                        else
                        {
                            merged.set(member.key, member.value);
                        }
                    }
                    aNode = std::move(merged);
                }

                ObjectDesc object;
                object.uid = aDocument.allocateUid();
                object.name = aName;

                const std::string legacyType = aNode.get("type").asString("Node");
                const ValueObject& params = aNode.get("params").asObject();
                const ValueObject& metadata = aNode.get("metadata").asObject();
                ValueObject frameSource = params;

                if (metadata.contains("spawn_entity"))
                {
                    object.type = "SpawnPoint";
                    object.props.set("prefab", Value("prefabs/" + toLower(metadata.get("spawn_entity").asString()) + ".json"));
                }
                else if (legacyType == "Sprite" || legacyType == "Button" || legacyType == "Label")
                {
                    object.type = legacyType;
                }
                else if (legacyType == "Entity")
                {
                    object.type = "Group";
                    for (const auto& component : aNode.get("components").asArray())
                    {
                        const ValueObject& componentObject = component.asObject();
                        const std::string componentType = componentObject.get("type").asString();
                        if (componentType == "SpriteComponent")
                        {
                            object.type = "Sprite";
                            for (const auto& member : componentObject)
                            {
                                frameSource.set(member.key, member.value);
                            }
                        }
                        else
                        {
                            mDiagnostics.warning(aPath + ".components", "legacy component '" + componentType + "' has no equivalent and was dropped");
                        }
                    }
                }
                else
                {
                    object.type = "Group";
                }

                if (params.contains("pos_x") || params.contains("pos_y"))
                {
                    object.props.set("position", writeVec2({params.get("pos_x").asFloat() * aParentSize.x, params.get("pos_y").asFloat() * aParentSize.y}));
                }
                if (params.contains("layer"))
                {
                    object.props.set("z", Value(params.get("layer").asInt()));
                }
                const float scaleX = params.get("scale_x").asFloat(1.0f);
                const float scaleY = params.get("scale_y").asFloat(1.0f);
                if (scaleX != 1.0f || scaleY != 1.0f)
                {
                    object.props.set("scale", writeVec2({scaleX, scaleY}));
                }
                for (const char* key : {"rotation", "opacity", "visible"})
                {
                    if (const Value* value = params.find(key))
                    {
                        object.props.set(key, *value);
                    }
                }

                if (object.type == "Sprite" || object.type == "Label")
                {
                    const Vec2 anchor{params.get("anch_x").asFloat(0.5f), params.get("anch_y").asFloat(0.5f)};
                    if (anchor != Vec2{0.5f, 0.5f})
                    {
                        object.props.set("anchor", writeVec2(anchor));
                    }
                }

                if (object.type == "Sprite")
                {
                    importSpriteImage(object, frameSource, aPath);
                }
                if (object.type == "Button")
                {
                    if (const Value* normal = params.find("res_normal"))
                    {
                        object.props.set("normal", *normal);
                    }
                    else if (const Value* res = params.find("res"))
                    {
                        object.props.set("normal", *res);
                    }
                    if (const Value* pressed = params.find("res_pressed"))
                    {
                        object.props.set("pressed", *pressed);
                    }
                }
                if (object.type == "Button" || object.type == "Label")
                {
                    for (const char* key : {"text", "font_size"})
                    {
                        if (const Value* value = params.find(key))
                        {
                            object.props.set(key, *value);
                        }
                    }
                }

                const ValueObject& actions = aNode.get("actions").asObject();
                for (const auto& member : actions)
                {
                    if (member.key == "onBtnClickUp" && object.type == "Button")
                    {
                        object.props.set("on_click", Value(importActions(member.value.asArray(), aDocument, aPath + ".actions")));
                    }
                    else
                    {
                        mDiagnostics.warning(aPath + ".actions." + member.key, "legacy action hook has no equivalent and was dropped");
                    }
                }

                const Vec2 childSize = readLegacySize(params.get("content_size"), aParentSize);
                for (const auto& member : aNode.get("children").asObject())
                {
                    object.children.push_back(importNode(member.key, member.value.asObject(), aDocument, childSize, aPath + ".children." + member.key));
                }
                return object;
            }

            void importSpriteImage(ObjectDesc& aObject, const ValueObject& aSource, const std::string& aPath)
            {
                if (const Value* frame = aSource.find("sprite_frame"))
                {
                    const std::string frameName = frame->asString();
                    if (mAtlasFrames.count(frameName))
                    {
                        aObject.props.set("frame", Value(frameName));
                        return;
                    }
                    if (auto texture = findTextureByName(frameName))
                    {
                        aObject.props.set("texture", Value(*texture));
                        mDiagnostics.warning(aPath, "sprite frame '" + frameName + "' is not in any atlas, using texture '" + *texture + "'");
                        return;
                    }
                    mDiagnostics.error(aPath, "sprite frame '" + frameName + "' cannot be resolved");
                    return;
                }
                if (const Value* res = aSource.find("res"))
                {
                    aObject.props.set("texture", *res);
                }
            }

            std::optional<std::string> findTextureByName(const std::string& aFileName)
            {
                for (const auto& file : mContentFiles)
                {
                    if (std::filesystem::path(file).filename() == aFileName)
                    {
                        return file;
                    }
                }
                return std::nullopt;
            }

            ValueArray importActions(const ValueArray& aActions, SceneDocument& aDocument, const std::string& aPath)
            {
                ValueArray result;
                for (const auto& action : aActions)
                {
                    const ValueObject& actionObject = action.asObject();
                    const std::string name = actionObject.get("runAction").asString();
                    if (name == "change_view")
                    {
                        result.push_back(Value(ValueObject{{"do", Value("change_scene")}, {"scene", Value(sceneIdFromViewId(actionObject.get("id").asString()))}}));
                    }
                    else if (name == "play_cutscene")
                    {
                        TimelineDesc timeline = importCutscene(actionObject.get("path").asString(), aDocument);
                        result.push_back(Value(ValueObject{{"do", Value("play_timeline")}, {"timeline", Value(timeline.id)}}));
                        aDocument.timelines.push_back(std::move(timeline));
                    }
                    else
                    {
                        mDiagnostics.warning(aPath, "legacy action '" + name + "' has no equivalent and was dropped");
                    }
                }
                return result;
            }

            ObjectId findObjectByName(const std::vector<ObjectDesc>& aObjects, const std::string& aName)
            {
                for (const auto& object : aObjects)
                {
                    if (object.name == aName)
                    {
                        return object.uid;
                    }
                    if (ObjectId found = findObjectByName(object.children, aName))
                    {
                        return found;
                    }
                }
                return kInvalidObjectId;
            }

            TrackDesc& propertyTrack(TimelineDesc& aTimeline, const TargetRef& aTarget, const std::string& aProperty)
            {
                for (auto& track : aTimeline.tracks)
                {
                    if (track.kind == TrackKind::Property && track.target == aTarget && track.property == aProperty)
                    {
                        return track;
                    }
                }
                TrackDesc track;
                track.target = aTarget;
                track.property = aProperty;
                aTimeline.tracks.push_back(std::move(track));
                return aTimeline.tracks.back();
            }

            TrackDesc& eventTrack(TimelineDesc& aTimeline)
            {
                for (auto& track : aTimeline.tracks)
                {
                    if (track.kind == TrackKind::Event)
                    {
                        return track;
                    }
                }
                TrackDesc track;
                track.kind = TrackKind::Event;
                aTimeline.tracks.push_back(std::move(track));
                return aTimeline.tracks.back();
            }

            TimelineDesc importCutscene(const std::string& aPath, SceneDocument& aDocument)
            {
                TimelineDesc timeline;
                timeline.id = stemOf(aPath);
                const ValueObject cutscene = loadLegacy(aPath).asObject();
                float time = 0.0f;

                for (const auto& stepValue : cutscene.get("sequence").asArray())
                {
                    const ValueObject& step = stepValue.asObject();
                    const std::string action = step.get("action").asString();
                    const float duration = std::max(0.0f, step.get("duration").asFloat(0.0f));
                    const bool waitForFinish = step.get("wait_for_finish").asBool(true);

                    if (action == "wait")
                    {
                        time += duration;
                    }
                    else if (action == "fade_in" || action == "fade_out")
                    {
                        const std::string target = step.get("target_node").asString();
                        const ObjectId uid = findObjectByName(aDocument.objects, target);
                        if (uid == kInvalidObjectId)
                        {
                            mDiagnostics.error(aPath, "cutscene target '" + target + "' was not found");
                            continue;
                        }
                        const bool fadeIn = action == "fade_in";
                        TrackDesc& track = propertyTrack(timeline, TargetRef::object(uid), "opacity");
                        track.keys.push_back({time, Value(fadeIn ? 0 : 255), {}});
                        track.keys.push_back({time + duration, Value(fadeIn ? 255 : 0), {}});
                        if (waitForFinish)
                        {
                            time += duration;
                        }
                    }
                    else if (action == "camera_zoom")
                    {
                        TrackDesc& track = propertyTrack(timeline, TargetRef::camera(), "zoom");
                        track.keys.push_back({time + duration, Value(step.get("zoom").asFloat(1.0f)), Easing::of(EaseType::SineInOut)});
                        time += duration;
                    }
                    else if (action == "camera_move")
                    {
                        TrackDesc& track = propertyTrack(timeline, TargetRef::camera(), "center");
                        track.keys.push_back({time + duration, step.get("pos"), Easing::of(EaseType::SineInOut)});
                        time += duration;
                    }
                    else if (action == "change_view")
                    {
                        eventTrack(timeline).events.push_back({time, ValueObject{{"do", Value("change_scene")}, {"scene", Value(sceneIdFromViewId(step.get("view_id").asString()))}}});
                    }
                    else if (action == "play_sound")
                    {
                        const bool music = step.get("type").asString() == "music";
                        eventTrack(timeline).events.push_back({time, ValueObject{{"do", Value(music ? "play_music" : "play_sound")}, {"file", step.get("file")}}});
                    }
                    else
                    {
                        mDiagnostics.warning(aPath, "cutscene step '" + action + "' has no equivalent and was dropped");
                    }
                }

                for (auto& track : timeline.tracks)
                {
                    sortKeyframes(track.keys);
                    sortEvents(track.events);
                }
                timeline.duration = std::max({time, timelineContentEnd(timeline), 0.1f});
                return timeline;
            }

            std::filesystem::path mLegacyRoot;
            std::filesystem::path mContentRoot;
            LegacyImportResult& mResult;
            Diagnostics& mDiagnostics;
            std::vector<std::string> mContentFiles;
            std::set<std::string> mAtlasFrames;
        };
    }

    LegacyImportResult importLegacyContent(const std::filesystem::path& aLegacyRoot, const std::filesystem::path& aContentRoot)
    {
        LegacyImportResult result;
        LegacyImporter importer(aLegacyRoot, aContentRoot, result);
        importer.run();
        return result;
    }
}
