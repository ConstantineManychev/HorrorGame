#include "doctest.h"

#include "Core/Animation/Easing.h"
#include "Core/Animation/Path.h"
#include "Core/Animation/TimelineEvaluator.h"
#include "Core/Base/ValueConvert.h"

using namespace hg;

TEST_SUITE("Easing")
{
    TEST_CASE("all easings start at zero and end at one")
    {
        for (EaseType type : allEaseTypes())
        {
            const Easing easing = Easing::of(type);
            CHECK(easing.apply(0.0f) == doctest::Approx(0.0f).epsilon(0.001));
            CHECK(easing.apply(1.0f) == doctest::Approx(1.0f).epsilon(0.001));
        }
    }

    TEST_CASE("linear bezier is identity")
    {
        Easing easing;
        easing.type = EaseType::Bezier;
        easing.bezier = {0.0f, 0.0f, 1.0f, 1.0f};
        for (float t = 0.0f; t <= 1.0f; t += 0.1f)
        {
            CHECK(easing.apply(t) == doctest::Approx(t).epsilon(0.001));
        }
    }

    TEST_CASE("names round trip")
    {
        for (EaseType type : allEaseTypes())
        {
            CHECK(parseEaseType(easeTypeName(type)) == type);
        }
        Easing custom;
        custom.type = EaseType::Bezier;
        custom.bezier = {0.1f, 0.2f, 0.3f, 0.4f};
        CHECK(readEasing(writeEasing(custom)) == custom);
    }
}

TEST_SUITE("Path")
{
    TEST_CASE("linear path samples by distance")
    {
        PathShape shape;
        shape.kind = PathKind::Linear;
        shape.points = {{{0.0f, 0.0f}, {}, {}}, {{100.0f, 0.0f}, {}, {}}, {{100.0f, 100.0f}, {}, {}}};
        PathSampler sampler(shape);

        CHECK(sampler.length() == doctest::Approx(200.0f));
        const PathSample middle = sampler.sampleNormalized(0.5f);
        CHECK(middle.position.x == doctest::Approx(100.0f));
        CHECK(middle.position.y == doctest::Approx(0.0f));
        const PathSample late = sampler.sampleAtDistance(150.0f);
        CHECK(late.position.y == doctest::Approx(50.0f));
        CHECK(late.tangent.y == doctest::Approx(1.0f));
    }

    TEST_CASE("catmull rom passes through control points")
    {
        PathShape shape;
        shape.kind = PathKind::CatmullRom;
        shape.points = {{{0.0f, 0.0f}, {}, {}}, {{50.0f, 80.0f}, {}, {}}, {{120.0f, 10.0f}, {}, {}}};
        CHECK(shape.evaluateSegment(0, 1.0f).x == doctest::Approx(50.0f));
        CHECK(shape.evaluateSegment(1, 0.0f).y == doctest::Approx(80.0f));
        CHECK(shape.evaluateSegment(1, 1.0f).x == doctest::Approx(120.0f));
    }

    TEST_CASE("bezier honours handles")
    {
        PathShape shape;
        shape.kind = PathKind::Bezier;
        shape.points = {{{0.0f, 0.0f}, {}, {0.0f, 100.0f}}, {{100.0f, 0.0f}, {0.0f, 100.0f}, {}}};
        const Vec2 middle = shape.evaluateSegment(0, 0.5f);
        CHECK(middle.x == doctest::Approx(50.0f));
        CHECK(middle.y == doctest::Approx(75.0f));
    }

    TEST_CASE("path props round trip")
    {
        PathShape shape;
        shape.kind = PathKind::Bezier;
        shape.closed = true;
        shape.points = {{{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}}, {{7.0f, 8.0f}, {}, {}}};
        ValueObject props;
        writePathShape(shape, props);
        CHECK(readPathShape(props) == shape);
    }
}

TEST_SUITE("Timeline")
{
    TEST_CASE("keyframes interpolate numbers and vectors and hold outside range")
    {
        std::vector<Keyframe> keys = {
            {1.0f, writeVec2({0.0f, 0.0f}), {}},
            {3.0f, writeVec2({100.0f, 50.0f}), Easing::of(EaseType::Linear)},
        };
        CHECK(readVec2(*sampleKeyframes(keys, 2.0f)) == Vec2{50.0f, 25.0f});
        CHECK(readVec2(*sampleKeyframes(keys, 0.0f)) == Vec2{0.0f, 0.0f});
        CHECK(readVec2(*sampleKeyframes(keys, 10.0f)) == Vec2{100.0f, 50.0f});
        CHECK_FALSE(sampleKeyframes({}, 1.0f).has_value());
    }

    TEST_CASE("non numeric values step")
    {
        std::vector<Keyframe> keys = {
            {0.0f, Value("a.png"), {}},
            {1.0f, Value("b.png"), {}},
        };
        CHECK(sampleKeyframes(keys, 0.5f)->asString() == "a.png");
        CHECK(sampleKeyframes(keys, 1.0f)->asString() == "b.png");
    }

    TEST_CASE("easing applies to the segment arriving at a key")
    {
        std::vector<Keyframe> keys = {
            {0.0f, Value(0.0), {}},
            {1.0f, Value(1.0), Easing::of(EaseType::Step)},
        };
        CHECK(sampleKeyframes(keys, 0.9f)->asFloat() == doctest::Approx(0.0f));
    }

    TEST_CASE("path progress respects start, ease and reverse")
    {
        TrackDesc track;
        track.kind = TrackKind::Path;
        track.start = 2.0f;
        track.duration = 4.0f;
        CHECK_FALSE(samplePathProgress(track, 1.0f).has_value());
        CHECK(*samplePathProgress(track, 4.0f) == doctest::Approx(0.5f));
        CHECK(*samplePathProgress(track, 100.0f) == doctest::Approx(1.0f));
        track.reverse = true;
        CHECK(*samplePathProgress(track, 3.0f) == doctest::Approx(0.75f));
    }

    TEST_CASE("events fire in half open intervals")
    {
        TrackDesc track;
        track.kind = TrackKind::Event;
        track.events = {{0.0f, {}}, {1.0f, {}}, {2.0f, {}}};
        std::vector<const TimelineEvent*> fired;
        collectEvents(track, -0.001f, 1.0f, fired);
        CHECK(fired.size() == 2);
        fired.clear();
        collectEvents(track, 1.0f, 2.0f, fired);
        CHECK(fired.size() == 1);
    }

    TEST_CASE("timeline serialization round trip")
    {
        TimelineDesc timeline;
        timeline.id = "intro";
        timeline.duration = 5.0f;
        timeline.loop = true;

        TrackDesc property;
        property.target = TargetRef::object(7);
        property.property = "opacity";
        property.keys = {{0.0f, Value(0), {}}, {2.0f, Value(255), Easing::of(EaseType::SineInOut)}};
        timeline.tracks.push_back(property);

        TrackDesc camera;
        camera.kind = TrackKind::Path;
        camera.target = TargetRef::camera();
        camera.path = 3;
        camera.start = 1.0f;
        camera.duration = 2.0f;
        camera.orient = true;
        timeline.tracks.push_back(camera);

        TrackDesc events;
        events.kind = TrackKind::Event;
        events.events = {{4.0f, ValueObject{{"do", Value("change_scene")}, {"scene", Value("menu")}}}};
        timeline.tracks.push_back(events);

        Diagnostics diagnostics;
        const TimelineDesc restored = readTimeline(writeTimeline(timeline), diagnostics, "timeline");
        CHECK(diagnostics.empty());
        CHECK(restored == timeline);
    }
}
