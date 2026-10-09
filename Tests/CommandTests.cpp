#include "doctest.h"

#include "Core/Base/ValueConvert.h"
#include "Core/Edit/EditSession.h"
#include "Core/Edit/SceneCommands.h"
#include "Core/Scene/SceneSerializer.h"

using namespace hg;

namespace
{
    ObjectDesc makeObject(ObjectId aUid, std::string aName)
    {
        ObjectDesc object;
        object.uid = aUid;
        object.name = std::move(aName);
        object.type = "Group";
        return object;
    }

    SceneDocument makeScene()
    {
        SceneDocument document = createEmptyScene("commands", SceneKind::Location);
        ObjectDesc parent = makeObject(1, "parent");
        parent.children.push_back(makeObject(2, "child"));
        document.objects.push_back(parent);
        document.objects.push_back(makeObject(3, "other"));
        document.nextUid = 4;
        return document;
    }

    std::vector<ObjectId> rootIds(const SceneDocument& aDocument)
    {
        std::vector<ObjectId> ids;
        for (const auto& object : aDocument.objects)
        {
            ids.push_back(object.uid);
        }
        return ids;
    }
}

TEST_SUITE("Commands")
{
    TEST_CASE("set property undo and redo")
    {
        EditSession session(makeScene());
        ChangeSet lastChanges;
        session.addListener([&lastChanges](const ChangeSet& aChanges)
        {
            lastChanges = aChanges;
        });

        REQUIRE(session.execute(std::make_unique<SetPropertyCommand>(2, "position", writeVec2({5.0f, 5.0f}))));
        CHECK(session.isDirty());
        CHECK(lastChanges.properties.size() == 1);
        CHECK(readVec2(session.document().findObject(2)->props.get("position")) == Vec2{5.0f, 5.0f});

        REQUIRE(session.undo());
        CHECK_FALSE(session.document().findObject(2)->props.contains("position"));
        CHECK_FALSE(session.isDirty());

        REQUIRE(session.redo());
        CHECK(session.document().findObject(2)->props.contains("position"));
    }

    TEST_CASE("drag gestures merge into one undo step")
    {
        EditSession session(makeScene());
        for (int step = 1; step <= 5; ++step)
        {
            session.execute(std::make_unique<SetPropertyCommand>(3, "position", writeVec2({static_cast<float>(step), 0.0f})), true);
        }
        session.closeMergeWindow();
        session.execute(std::make_unique<SetPropertyCommand>(3, "position", writeVec2({100.0f, 0.0f})), true);

        CHECK(readVec2(session.document().findObject(3)->props.get("position"))->x == doctest::Approx(100.0f));
        REQUIRE(session.undo());
        CHECK(readVec2(session.document().findObject(3)->props.get("position"))->x == doctest::Approx(5.0f));
        REQUIRE(session.undo());
        CHECK_FALSE(session.document().findObject(3)->props.contains("position"));
        CHECK_FALSE(session.canUndo());
    }

    TEST_CASE("delete restores object at the same place")
    {
        EditSession session(makeScene());
        REQUIRE(session.execute(std::make_unique<DeleteObjectCommand>(1)));
        CHECK(session.document().findObject(2) == nullptr);
        REQUIRE(session.undo());
        CHECK(rootIds(session.document()) == std::vector<ObjectId>{1, 3});
        CHECK(session.document().findParent(2)->uid == 1);
    }

    TEST_CASE("move reorders, reparents and refuses cycles")
    {
        EditSession session(makeScene());
        REQUIRE(session.execute(std::make_unique<MoveObjectCommand>(3, kInvalidObjectId, 0)));
        CHECK(rootIds(session.document()) == std::vector<ObjectId>{3, 1});
        REQUIRE(session.undo());
        CHECK(rootIds(session.document()) == std::vector<ObjectId>{1, 3});

        REQUIRE(session.execute(std::make_unique<MoveObjectCommand>(2, 3, 0)));
        CHECK(session.document().findParent(2)->uid == 3);
        REQUIRE(session.undo());
        CHECK(session.document().findParent(2)->uid == 1);

        CHECK_FALSE(session.execute(std::make_unique<MoveObjectCommand>(1, 2, 0)));
        CHECK_FALSE(session.execute(std::make_unique<MoveObjectCommand>(1, kInvalidObjectId, 1)));
    }

    TEST_CASE("create assigns position and keeps uid counter ahead")
    {
        SceneDocument document = makeScene();
        ObjectDesc created = makeObject(document.allocateUid(), "new");
        EditSession session(std::move(document));
        REQUIRE(session.execute(std::make_unique<CreateObjectCommand>(1, 0, created)));
        CHECK(session.document().findParent(created.uid)->uid == 1);
        CHECK(session.document().objects[0].children.front().uid == created.uid);
        REQUIRE(session.undo());
        CHECK(session.document().findObject(created.uid) == nullptr);
    }

    TEST_CASE("components and settings")
    {
        EditSession session(makeScene());
        REQUIRE(session.execute(std::make_unique<AddComponentCommand>(3, ComponentDesc{"CharacterBody", {}})));
        CHECK_FALSE(session.execute(std::make_unique<AddComponentCommand>(3, ComponentDesc{"CharacterBody", {}})));
        REQUIRE(session.execute(std::make_unique<SetComponentPropertyCommand>(3, 0, "size", writeVec2({1.0f, 2.0f}))));
        REQUIRE(session.execute(std::make_unique<RemoveComponentCommand>(3, 0)));
        CHECK(session.document().findObject(3)->components.empty());
        REQUIRE(session.undo());
        CHECK(session.document().findObject(3)->components[0].props.contains("size"));

        REQUIRE(session.execute(std::make_unique<SetSettingCommand>("control_mode", Value("top_down"))));
        CHECK(session.document().settings.get("control_mode").asString() == "top_down");
        REQUIRE(session.undo());
        CHECK(session.document().settings.get("control_mode").asString() == "side_scroll");
    }

    TEST_CASE("composite command is atomic")
    {
        EditSession session(makeScene());
        auto composite = std::make_unique<CompositeCommand>("Batch");
        composite->add(std::make_unique<SetPropertyCommand>(1, "z", Value(4)));
        composite->add(std::make_unique<SetPropertyCommand>(999, "z", Value(4)));
        CHECK_FALSE(session.execute(std::move(composite)));
        CHECK_FALSE(session.document().findObject(1)->props.contains("z"));
    }

    TEST_CASE("timelines snapshot command")
    {
        EditSession session(makeScene());
        std::vector<TimelineDesc> timelines(1);
        timelines[0].id = "intro";
        REQUIRE(session.execute(std::make_unique<SetTimelinesCommand>(timelines, "Add timeline")));
        CHECK(session.document().findTimeline("intro") != nullptr);
        REQUIRE(session.undo());
        CHECK(session.document().timelines.empty());
    }

    TEST_CASE("saving after a merge keeps dirty state correct")
    {
        EditSession session(makeScene());
        session.execute(std::make_unique<SetPropertyCommand>(3, "z", Value(1)), true);
        session.markSaved();
        CHECK_FALSE(session.isDirty());
        session.execute(std::make_unique<SetPropertyCommand>(3, "z", Value(2)), true);
        CHECK(session.isDirty());
    }
}
