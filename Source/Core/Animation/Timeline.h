#pragma once

#include "Core/Animation/Easing.h"
#include "Core/Base/Diagnostics.h"
#include "Core/Base/Ids.h"
#include "Core/Base/Value.h"

#include <optional>
#include <string>
#include <vector>

namespace hg
{
    enum class TrackKind
    {
        Property,
        Path,
        Event
    };

    struct TargetRef
    {
        enum class Kind
        {
            None,
            Object,
            Camera
        };

        Kind kind = Kind::None;
        ObjectId uid = kInvalidObjectId;

        static TargetRef object(ObjectId aUid);
        static TargetRef camera();

        bool isObject() const;
        bool isCamera() const;

        bool operator==(const TargetRef& aOther) const = default;
    };

    struct Keyframe
    {
        float time = 0.0f;
        Value value;
        Easing ease;

        bool operator==(const Keyframe& aOther) const = default;
    };

    struct TimelineEvent
    {
        float time = 0.0f;
        ValueObject action;

        bool operator==(const TimelineEvent& aOther) const = default;
    };

    struct TrackDesc
    {
        TrackKind kind = TrackKind::Property;
        TargetRef target;
        std::string property;
        std::vector<Keyframe> keys;
        ObjectId path = kInvalidObjectId;
        float start = 0.0f;
        float duration = 1.0f;
        Easing ease;
        bool orient = false;
        bool reverse = false;
        std::vector<TimelineEvent> events;
        bool muted = false;

        bool operator==(const TrackDesc& aOther) const = default;
    };

    struct TimelineDesc
    {
        std::string id;
        float duration = 1.0f;
        bool loop = false;
        std::vector<TrackDesc> tracks;

        bool operator==(const TimelineDesc& aOther) const = default;
    };

    std::string_view trackKindName(TrackKind aKind);
    std::optional<TrackKind> parseTrackKind(std::string_view aName);

    TimelineDesc readTimeline(const Value& aValue, Diagnostics& aDiagnostics, const std::string& aPath);
    Value writeTimeline(const TimelineDesc& aTimeline);

    void sortKeyframes(std::vector<Keyframe>& aKeys);
    void sortEvents(std::vector<TimelineEvent>& aEvents);
    float trackEndTime(const TrackDesc& aTrack);
    float timelineContentEnd(const TimelineDesc& aTimeline);
}
