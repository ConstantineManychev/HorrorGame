#include "doctest.h"

#include "Core/Base/Json.h"
#include "Core/Base/ValueConvert.h"
#include "Core/Scene/Prefab.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Scene/Validator.h"

#include <map>

using namespace hg;

namespace
{
    ObjectDesc makeObject(ObjectId aUid, std::string aName, std::string aType)
    {
        ObjectDesc object;
        object.uid = aUid;
        object.name = std::move(aName);
        object.type = std::move(aType);
        return object;
    }

    SceneDocument makeScene()
    {
        SceneDocument document = createEmptyScene("test", SceneKind::Location);
        ObjectDesc root = makeObject(1, "root", "Group");
        ObjectDesc sprite = makeObject(2, "sprite", "Sprite");
        sprite.props.set("position", writeVec2({10.0f, 20.0f}));
        sprite.props.set("custom_tag", Value("kept"));
        root.children.push_back(sprite);
        document.objects.push_back(root);
        document.objects.push_back(makeObject(3, "path", "Path"));
        return document;
    }

    TextFileReader readerFor(std::map<std::string, std::string> aFiles)
    {
        return [files = std::move(aFiles)](const std::string& aPath) -> std::optional<std::string>
        {
            auto it = files.find(aPath);
            return it != files.end() ? std::optional<std::string>(it->second) : std::nullopt;
        };
    }
}

TEST_SUITE("SceneSerializer")
{
    TEST_CASE("scene round trip keeps unknown data")
    {
        SceneDocument document = makeScene();
        document.extra.set("author_note", Value("hello"));

        Diagnostics diagnostics;
        auto restored = readScene(writeScene(document), diagnostics);
        REQUIRE(restored.has_value());
        CHECK_FALSE(diagnostics.hasErrors());
        CHECK(restored->objects == document.objects);
        CHECK(restored->settings == document.settings);
        CHECK(restored->extra.get("author_note").asString() == "hello");
        CHECK(restored->findObject(2)->props.get("custom_tag").asString() == "kept");
    }

    TEST_CASE("legacy and future versions are rejected")
    {
        Diagnostics diagnostics;
        CHECK_FALSE(readScene(Value(ValueObject{{"id", Value("old")}}), diagnostics).has_value());
        CHECK(diagnostics.hasErrors());

        Diagnostics future;
        CHECK_FALSE(readScene(Value(ValueObject{{"version", Value(99)}, {"id", Value("x")}}), future).has_value());
        CHECK(future.hasErrors());
    }

    TEST_CASE("duplicate and missing uids are repaired on load")
    {
        SceneDocument document = makeScene();
        document.objects.push_back(makeObject(1, "dup", "Group"));
        document.objects.push_back(makeObject(0, "none", "Group"));

        Diagnostics diagnostics;
        auto restored = readScene(writeScene(document), diagnostics);
        REQUIRE(restored.has_value());
        CHECK(restored->objects[2].uid != 1);
        CHECK(restored->objects[3].uid != 0);
        CHECK(restored->nextUid > restored->computeMaxUid());
    }

    TEST_CASE("document queries")
    {
        SceneDocument document = makeScene();
        CHECK(document.findParent(2)->uid == 1);
        CHECK(document.findParent(1) == nullptr);
        CHECK(document.isAncestor(1, 2));
        CHECK_FALSE(document.isAncestor(2, 1));
        CHECK(document.allocateUid() == 4);
        CHECK(makeUniqueName(document.objects, "root") == "root_1");
    }
}

TEST_SUITE("Prefab")
{
    TEST_CASE("instance overrides props and merges components")
    {
        ObjectDesc prefab = makeObject(1, "player", "Group");
        prefab.props.set("z", Value(5));
        prefab.components.push_back({"CharacterBody", ValueObject{{"size", writeVec2({10.0f, 20.0f})}}});
        prefab.children.push_back(makeObject(2, "body", "ColorRect"));

        ObjectDesc instance;
        instance.uid = 42;
        instance.name = "hero";
        instance.prefab = "prefabs/player.json";
        instance.props.set("position", writeVec2({5.0f, 6.0f}));
        instance.components.push_back({"CharacterBody", ValueObject{{"offset", writeVec2({0.0f, 1.0f})}}});

        ObjectDesc merged = mergePrefabInstance(instance, prefab);
        CHECK(merged.uid == 42);
        CHECK(merged.name == "hero");
        CHECK(merged.type == "Group");
        CHECK(merged.props.get("z").asInt() == 5);
        CHECK(readVec2(merged.props.get("position")) == Vec2{5.0f, 6.0f});
        REQUIRE(merged.components.size() == 1);
        CHECK(merged.components[0].props.contains("size"));
        CHECK(merged.components[0].props.contains("offset"));
        CHECK(merged.children[0].uid == combinePrefabIds(42, 2));
    }

    TEST_CASE("expansion loads prefabs through the library")
    {
        ObjectDesc prefabRoot = makeObject(1, "lamp", "Sprite");
        PrefabLibrary library(readerFor({{"prefabs/lamp.json", writeJson(writePrefab(prefabRoot))}}));

        ObjectDesc instance;
        instance.uid = 9;
        instance.name = "lamp_a";
        instance.prefab = "prefabs/lamp.json";

        Diagnostics diagnostics;
        ObjectDesc expanded = expandPrefabs(instance, library, diagnostics);
        CHECK_FALSE(diagnostics.hasErrors());
        CHECK(expanded.type == "Sprite");
        CHECK(expanded.prefab.empty());

        instance.prefab = "prefabs/missing.json";
        Diagnostics missing;
        expandPrefabs(instance, library, missing);
        CHECK(missing.hasErrors());
    }
}

TEST_SUITE("Validator")
{
    TEST_CASE("valid scene has no errors")
    {
        Diagnostics diagnostics;
        validateScene(makeScene(), {}, diagnostics);
        for (const auto& entry : diagnostics.entries())
        {
            INFO(formatDiagnostic(entry));
        }
        CHECK_FALSE(diagnostics.hasErrors());
        CHECK(diagnostics.count(Severity::Warning) == 1);
    }

    TEST_CASE("detects unknown types, bad values and broken references")
    {
        SceneDocument document = makeScene();
        document.objects.push_back(makeObject(10, "ghost", "Ghost"));
        document.findObject(2)->props.set("opacity", Value("loud"));

        ObjectDesc follower = makeObject(11, "follower", "Group");
        follower.components.push_back({"PathFollower", ValueObject{{"path", Value(2)}}});
        document.objects.push_back(follower);

        ObjectDesc button = makeObject(12, "button", "Button");
        button.props.set("on_click", Value(ValueArray{Value(ValueObject{{"do", Value("teleport")}})}));
        document.objects.push_back(button);

        TimelineDesc timeline;
        timeline.id = "broken";
        TrackDesc track;
        track.target = TargetRef::object(999);
        track.property = "position";
        timeline.tracks.push_back(track);
        TrackDesc locked;
        locked.target = TargetRef::object(2);
        locked.property = "z";
        timeline.tracks.push_back(locked);
        document.timelines.push_back(timeline);

        Diagnostics diagnostics;
        validateScene(document, {}, diagnostics);

        auto mentions = [&diagnostics](const std::string& aText)
        {
            for (const auto& entry : diagnostics.entries())
            {
                if (entry.severity == Severity::Error && entry.message.find(aText) != std::string::npos)
                {
                    return true;
                }
            }
            return false;
        };

        CHECK(mentions("unknown object type 'Ghost'"));
        CHECK(mentions("does not match type int"));
        CHECK(mentions("must be of type 'Path'"));
        CHECK(mentions("unknown action 'teleport'"));
        CHECK(mentions("target object 999 does not exist"));
        CHECK(mentions("is not animatable"));
    }

    TEST_CASE("asset checks use the provided callback")
    {
        SceneDocument document = makeScene();
        document.findObject(2)->props.set("texture", Value("res/missing.png"));

        ValidationContext context;
        context.assetExists = [](const std::string& aPath)
        {
            return aPath != "res/missing.png";
        };

        Diagnostics diagnostics;
        validateScene(document, context, diagnostics);
        CHECK(diagnostics.hasErrors());
    }
}
