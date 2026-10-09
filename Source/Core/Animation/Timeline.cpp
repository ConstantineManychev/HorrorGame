#include "Core/Animation/Timeline.h"

#include <algorithm>

namespace hg
{
    namespace
    {
        constexpr std::string_view kCameraTarget = "@camera";

        TargetRef readTarget(const Value& aValue)
        {
            if (aValue.isString() && aValue.asString() == kCameraTarget)
            {
                return TargetRef::camera();
            }
            if (aValue.isInt() && aValue.asInt() > 0)
            {
                return TargetRef::object(static_cast<ObjectId>(aValue.asInt()));
            }
            return {};
        }

        Value writeTarget(const TargetRef& aTarget)
        {
            switch (aTarget.kind)
            {
            case TargetRef::Kind::Camera:
                return Value(kCameraTarget);
            case TargetRef::Kind::Object:
                return Value(aTarget.uid);
            case TargetRef::Kind::None:
                return Value();
            }
            return Value();
        }
    }

    TargetRef TargetRef::object(ObjectId aUid)
    {
        return {Kind::Object, aUid};
    }

    TargetRef TargetRef::camera()
    {
        return {Kind::Camera, kInvalidObjectId};
    }

    bool TargetRef::isObject() const
    {
        return kind == Kind::Object;
    }

    bool TargetRef::isCamera() const
    {
        return kind == Kind::Camera;
    }

    std::string_view trackKindName(TrackKind aKind)
    {
        switch (aKind)
        {
        case TrackKind::Property:
            return "property";
        case TrackKind::Path:
            return "path";
        case TrackKind::Event:
            return "events";
        }
        return "property";
    }

    std::optional<TrackKind> parseTrackKind(std::string_view aName)
    {
        if (aName == "property")
        {
            return TrackKind::Property;
        }
        if (aName == "path")
        {
            return TrackKind::Path;
        }
        if (aName == "events")
        {
            return TrackKind::Event;
        }
        return std::nullopt;
    }

    TimelineDesc readTimeline(const Value& aValue, Diagnostics& aDiagnostics, const std::string& aPath)
    {
        TimelineDesc timeline;
        const ValueObject& object = aValue.asObject();
        timeline.id = object.get("id").asString();
        timeline.duration = std::max(0.0f, object.get("duration").asFloat(1.0f));
        timeline.loop = object.get("loop").asBool(false);
        if (timeline.id.empty())
        {
            aDiagnostics.error(aPath, "timeline has no id");
        }

        const ValueArray& tracks = object.get("tracks").asArray();
        for (size_t trackIndex = 0; trackIndex < tracks.size(); ++trackIndex)
        {
            const std::string trackPath = aPath + ".tracks[" + std::to_string(trackIndex) + "]";
            const ValueObject& trackObject = tracks[trackIndex].asObject();
            TrackDesc track;

            const std::string kindName = trackObject.get("kind").asString("property");
            auto kind = parseTrackKind(kindName);
            if (!kind)
            {
                aDiagnostics.error(trackPath, "unknown track kind '" + kindName + "'");
                continue;
            }
            track.kind = *kind;
            track.target = readTarget(trackObject.get("target"));
            track.muted = trackObject.get("muted").asBool(false);

            switch (track.kind)
            {
            case TrackKind::Property:
            {
                track.property = trackObject.get("property").asString();
                const ValueArray& keys = trackObject.get("keys").asArray();
                for (const auto& keyValue : keys)
                {
                    const ValueObject& keyObject = keyValue.asObject();
                    Keyframe key;
                    key.time = keyObject.get("t").asFloat();
                    key.value = keyObject.get("v");
                    key.ease = readEasing(keyObject.get("ease"));
                    track.keys.push_back(std::move(key));
                }
                sortKeyframes(track.keys);
                break;
            }
            case TrackKind::Path:
                track.path = static_cast<ObjectId>(std::max<int64_t>(0, trackObject.get("path").asInt()));
                track.start = trackObject.get("start").asFloat(0.0f);
                track.duration = std::max(0.001f, trackObject.get("duration").asFloat(1.0f));
                track.ease = readEasing(trackObject.get("ease"));
                track.orient = trackObject.get("orient").asBool(false);
                track.reverse = trackObject.get("reverse").asBool(false);
                break;
            case TrackKind::Event:
            {
                const ValueArray& events = trackObject.get("events").asArray();
                for (const auto& eventValue : events)
                {
                    const ValueObject& eventObject = eventValue.asObject();
                    TimelineEvent event;
                    event.time = eventObject.get("t").asFloat();
                    event.action = eventObject.get("action").asObject();
                    track.events.push_back(std::move(event));
                }
                sortEvents(track.events);
                break;
            }
            }
            timeline.tracks.push_back(std::move(track));
        }
        return timeline;
    }

    Value writeTimeline(const TimelineDesc& aTimeline)
    {
        ValueObject object;
        object.set("id", Value(aTimeline.id));
        object.set("duration", Value(aTimeline.duration));
        if (aTimeline.loop)
        {
            object.set("loop", Value(true));
        }

        ValueArray tracks;
        for (const auto& track : aTimeline.tracks)
        {
            ValueObject trackObject;
            trackObject.set("kind", Value(trackKindName(track.kind)));
            if (track.kind != TrackKind::Event)
            {
                trackObject.set("target", writeTarget(track.target));
            }
            if (track.muted)
            {
                trackObject.set("muted", Value(true));
            }
            switch (track.kind)
            {
            case TrackKind::Property:
            {
                trackObject.set("property", Value(track.property));
                ValueArray keys;
                for (const auto& key : track.keys)
                {
                    ValueObject keyObject;
                    keyObject.set("t", Value(key.time));
                    keyObject.set("v", key.value);
                    if (key.ease.type != EaseType::Linear)
                    {
                        keyObject.set("ease", writeEasing(key.ease));
                    }
                    keys.emplace_back(std::move(keyObject));
                }
                trackObject.set("keys", Value(std::move(keys)));
                break;
            }
            case TrackKind::Path:
                trackObject.set("path", Value(track.path));
                trackObject.set("start", Value(track.start));
                trackObject.set("duration", Value(track.duration));
                if (track.ease.type != EaseType::Linear)
                {
                    trackObject.set("ease", writeEasing(track.ease));
                }
                if (track.orient)
                {
                    trackObject.set("orient", Value(true));
                }
                if (track.reverse)
                {
                    trackObject.set("reverse", Value(true));
                }
                break;
            case TrackKind::Event:
            {
                ValueArray events;
                for (const auto& event : track.events)
                {
                    ValueObject eventObject;
                    eventObject.set("t", Value(event.time));
                    eventObject.set("action", Value(event.action));
                    events.emplace_back(std::move(eventObject));
                }
                trackObject.set("events", Value(std::move(events)));
                break;
            }
            }
            tracks.emplace_back(std::move(trackObject));
        }
        object.set("tracks", Value(std::move(tracks)));
        return Value(std::move(object));
    }

    void sortKeyframes(std::vector<Keyframe>& aKeys)
    {
        std::stable_sort(aKeys.begin(), aKeys.end(), [](const Keyframe& aLeft, const Keyframe& aRight)
        {
            return aLeft.time < aRight.time;
        });
    }

    void sortEvents(std::vector<TimelineEvent>& aEvents)
    {
        std::stable_sort(aEvents.begin(), aEvents.end(), [](const TimelineEvent& aLeft, const TimelineEvent& aRight)
        {
            return aLeft.time < aRight.time;
        });
    }

    float trackEndTime(const TrackDesc& aTrack)
    {
        switch (aTrack.kind)
        {
        case TrackKind::Property:
            return aTrack.keys.empty() ? 0.0f : aTrack.keys.back().time;
        case TrackKind::Path:
            return aTrack.start + aTrack.duration;
        case TrackKind::Event:
            return aTrack.events.empty() ? 0.0f : aTrack.events.back().time;
        }
        return 0.0f;
    }

    float timelineContentEnd(const TimelineDesc& aTimeline)
    {
        float end = 0.0f;
        for (const auto& track : aTimeline.tracks)
        {
            end = std::max(end, trackEndTime(track));
        }
        return end;
    }
}
