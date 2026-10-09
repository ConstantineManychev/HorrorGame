#include "Core/Animation/TimelineEvaluator.h"

#include "Core/Base/Math.h"

#include <algorithm>

namespace hg
{
    namespace
    {
        bool isNumericArray(const Value& aValue)
        {
            const ValueArray* array = aValue.getArray();
            if (!array)
            {
                return false;
            }
            return std::all_of(array->begin(), array->end(), [](const Value& aElement)
            {
                return aElement.isNumber();
            });
        }
    }

    Value interpolateValues(const Value& aFrom, const Value& aTo, float aT)
    {
        if (aFrom.isNumber() && aTo.isNumber())
        {
            return Value(static_cast<double>(lerp(aFrom.asFloat(), aTo.asFloat(), aT)));
        }
        if (isNumericArray(aFrom) && isNumericArray(aTo) && aFrom.asArray().size() == aTo.asArray().size())
        {
            const ValueArray& from = aFrom.asArray();
            const ValueArray& to = aTo.asArray();
            ValueArray result;
            result.reserve(from.size());
            for (size_t index = 0; index < from.size(); ++index)
            {
                result.emplace_back(static_cast<double>(lerp(from[index].asFloat(), to[index].asFloat(), aT)));
            }
            return Value(std::move(result));
        }
        return aT >= 1.0f ? aTo : aFrom;
    }

    std::optional<Value> sampleKeyframes(const std::vector<Keyframe>& aKeys, float aTime)
    {
        if (aKeys.empty())
        {
            return std::nullopt;
        }
        if (aTime <= aKeys.front().time)
        {
            return aKeys.front().value;
        }
        if (aTime >= aKeys.back().time)
        {
            return aKeys.back().value;
        }

        auto next = std::upper_bound(aKeys.begin(), aKeys.end(), aTime, [](float aValue, const Keyframe& aKey)
        {
            return aValue < aKey.time;
        });
        auto previous = next - 1;
        const float span = next->time - previous->time;
        const float linearT = span > 0.0f ? (aTime - previous->time) / span : 1.0f;
        return interpolateValues(previous->value, next->value, next->ease.apply(linearT));
    }

    std::optional<float> samplePathProgress(const TrackDesc& aTrack, float aTime)
    {
        if (aTrack.kind != TrackKind::Path || aTime < aTrack.start)
        {
            return std::nullopt;
        }
        const float linearT = aTrack.duration > 0.0f ? clamp01((aTime - aTrack.start) / aTrack.duration) : 1.0f;
        const float eased = aTrack.ease.apply(linearT);
        return aTrack.reverse ? 1.0f - eased : eased;
    }

    void collectEvents(const TrackDesc& aTrack, float aFromExclusive, float aToInclusive, std::vector<const TimelineEvent*>& aOut)
    {
        if (aTrack.kind != TrackKind::Event || aTrack.muted)
        {
            return;
        }
        for (const auto& event : aTrack.events)
        {
            if (event.time > aFromExclusive && event.time <= aToInclusive)
            {
                aOut.push_back(&event);
            }
        }
    }
}
