#pragma once

#include "Core/Base/Math.h"
#include "Core/Base/Value.h"

#include <optional>
#include <string_view>
#include <vector>

namespace hg
{
    enum class PathKind
    {
        Linear,
        CatmullRom,
        Bezier
    };

    struct PathPoint
    {
        Vec2 position;
        Vec2 handleIn;
        Vec2 handleOut;

        bool operator==(const PathPoint& aOther) const = default;
    };

    struct PathShape
    {
        PathKind kind = PathKind::CatmullRom;
        std::vector<PathPoint> points;
        bool closed = false;

        size_t segmentCount() const;
        Vec2 evaluateSegment(size_t aSegment, float aT) const;

        bool operator==(const PathShape& aOther) const = default;
    };

    struct PathSample
    {
        Vec2 position;
        Vec2 tangent;
    };

    class PathSampler
    {
    public:
        PathSampler() = default;
        explicit PathSampler(const PathShape& aShape, int aSamplesPerSegment = 24);

        float length() const;
        bool empty() const;
        PathSample sampleAtDistance(float aDistance) const;
        PathSample sampleNormalized(float aU) const;
        const std::vector<Vec2>& polyline() const;

    private:
        std::vector<Vec2> mPoints;
        std::vector<float> mDistances;
    };

    std::string_view pathKindName(PathKind aKind);
    std::optional<PathKind> parsePathKind(std::string_view aName);

    PathShape readPathShape(const ValueObject& aProps);
    void writePathShape(const PathShape& aShape, ValueObject& aProps);
}
