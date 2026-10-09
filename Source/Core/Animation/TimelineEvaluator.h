#pragma once

#include "Core/Animation/Timeline.h"

#include <optional>
#include <vector>

namespace hg
{
    Value interpolateValues(const Value& aFrom, const Value& aTo, float aT);
    std::optional<Value> sampleKeyframes(const std::vector<Keyframe>& aKeys, float aTime);
    std::optional<float> samplePathProgress(const TrackDesc& aTrack, float aTime);
    void collectEvents(const TrackDesc& aTrack, float aFromExclusive, float aToInclusive, std::vector<const TimelineEvent*>& aOut);
}
