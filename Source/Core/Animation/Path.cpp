#include "Core/Animation/Path.h"

#include "Core/Base/ValueConvert.h"

#include <algorithm>

namespace hg
{
    namespace
    {
        Vec2 catmullRom(const Vec2& aP0, const Vec2& aP1, const Vec2& aP2, const Vec2& aP3, float aT)
        {
            const float t2 = aT * aT;
            const float t3 = t2 * aT;
            return (aP1 * 2.0f + (aP2 - aP0) * aT + (aP0 * 2.0f - aP1 * 5.0f + aP2 * 4.0f - aP3) * t2 + (aP1 * 3.0f - aP0 - aP2 * 3.0f + aP3) * t3) * 0.5f;
        }

        Vec2 cubicBezier(const Vec2& aP0, const Vec2& aP1, const Vec2& aP2, const Vec2& aP3, float aT)
        {
            const float inverse = 1.0f - aT;
            return aP0 * (inverse * inverse * inverse) + aP1 * (3.0f * inverse * inverse * aT) + aP2 * (3.0f * inverse * aT * aT) + aP3 * (aT * aT * aT);
        }
    }

    size_t PathShape::segmentCount() const
    {
        if (points.size() < 2)
        {
            return 0;
        }
        return closed ? points.size() : points.size() - 1;
    }

    Vec2 PathShape::evaluateSegment(size_t aSegment, float aT) const
    {
        const size_t count = points.size();
        if (count == 0)
        {
            return {};
        }
        if (count == 1)
        {
            return points.front().position;
        }

        auto pointAt = [this, count](long aIndex) -> const PathPoint&
        {
            if (closed)
            {
                const long wrapped = ((aIndex % static_cast<long>(count)) + static_cast<long>(count)) % static_cast<long>(count);
                return points[static_cast<size_t>(wrapped)];
            }
            return points[static_cast<size_t>(std::clamp<long>(aIndex, 0, static_cast<long>(count) - 1))];
        };

        const long index = static_cast<long>(aSegment);
        const PathPoint& start = pointAt(index);
        const PathPoint& end = pointAt(index + 1);

        switch (kind)
        {
        case PathKind::Linear:
            return Vec2::lerp(start.position, end.position, aT);
        case PathKind::CatmullRom:
            return catmullRom(pointAt(index - 1).position, start.position, end.position, pointAt(index + 2).position, aT);
        case PathKind::Bezier:
            return cubicBezier(start.position, start.position + start.handleOut, end.position + end.handleIn, end.position, aT);
        }
        return start.position;
    }

    PathSampler::PathSampler(const PathShape& aShape, int aSamplesPerSegment)
    {
        const size_t segments = aShape.segmentCount();
        if (segments == 0)
        {
            if (!aShape.points.empty())
            {
                mPoints.push_back(aShape.points.front().position);
                mDistances.push_back(0.0f);
            }
            return;
        }

        const int samples = aShape.kind == PathKind::Linear ? 1 : std::max(aSamplesPerSegment, 2);
        mPoints.reserve(segments * static_cast<size_t>(samples) + 1);
        mPoints.push_back(aShape.evaluateSegment(0, 0.0f));
        for (size_t segment = 0; segment < segments; ++segment)
        {
            for (int step = 1; step <= samples; ++step)
            {
                mPoints.push_back(aShape.evaluateSegment(segment, static_cast<float>(step) / static_cast<float>(samples)));
            }
        }

        mDistances.reserve(mPoints.size());
        mDistances.push_back(0.0f);
        for (size_t index = 1; index < mPoints.size(); ++index)
        {
            mDistances.push_back(mDistances.back() + mPoints[index].distance(mPoints[index - 1]));
        }
    }

    float PathSampler::length() const
    {
        return mDistances.empty() ? 0.0f : mDistances.back();
    }

    bool PathSampler::empty() const
    {
        return mPoints.empty();
    }

    PathSample PathSampler::sampleAtDistance(float aDistance) const
    {
        if (mPoints.empty())
        {
            return {};
        }
        if (mPoints.size() == 1)
        {
            return {mPoints.front(), {1.0f, 0.0f}};
        }

        const float distance = std::clamp(aDistance, 0.0f, length());
        auto upper = std::lower_bound(mDistances.begin(), mDistances.end(), distance);
        size_t index = static_cast<size_t>(std::distance(mDistances.begin(), upper));
        index = std::clamp<size_t>(index, 1, mPoints.size() - 1);

        const float segmentStart = mDistances[index - 1];
        const float segmentLength = mDistances[index] - segmentStart;
        const float t = segmentLength > 0.0f ? (distance - segmentStart) / segmentLength : 0.0f;

        PathSample sample;
        sample.position = Vec2::lerp(mPoints[index - 1], mPoints[index], t);
        sample.tangent = (mPoints[index] - mPoints[index - 1]).normalized();
        return sample;
    }

    PathSample PathSampler::sampleNormalized(float aU) const
    {
        return sampleAtDistance(clamp01(aU) * length());
    }

    const std::vector<Vec2>& PathSampler::polyline() const
    {
        return mPoints;
    }

    std::string_view pathKindName(PathKind aKind)
    {
        switch (aKind)
        {
        case PathKind::Linear:
            return "linear";
        case PathKind::CatmullRom:
            return "catmull_rom";
        case PathKind::Bezier:
            return "bezier";
        }
        return "linear";
    }

    std::optional<PathKind> parsePathKind(std::string_view aName)
    {
        if (aName == "linear")
        {
            return PathKind::Linear;
        }
        if (aName == "catmull_rom")
        {
            return PathKind::CatmullRom;
        }
        if (aName == "bezier")
        {
            return PathKind::Bezier;
        }
        return std::nullopt;
    }

    PathShape readPathShape(const ValueObject& aProps)
    {
        PathShape shape;
        shape.kind = parsePathKind(aProps.get("kind").asString()).value_or(PathKind::CatmullRom);
        shape.closed = aProps.get("closed").asBool(false);

        const std::vector<Vec2> positions = readVec2List(aProps.get("points"));
        const ValueArray& handles = aProps.get("handles").asArray();
        shape.points.reserve(positions.size());
        for (size_t index = 0; index < positions.size(); ++index)
        {
            PathPoint point;
            point.position = positions[index];
            if (index < handles.size())
            {
                const ValueArray& handle = handles[index].asArray();
                if (handle.size() == 4)
                {
                    point.handleIn = {handle[0].asFloat(), handle[1].asFloat()};
                    point.handleOut = {handle[2].asFloat(), handle[3].asFloat()};
                }
            }
            shape.points.push_back(point);
        }
        return shape;
    }

    void writePathShape(const PathShape& aShape, ValueObject& aProps)
    {
        aProps.set("kind", Value(pathKindName(aShape.kind)));
        aProps.set("closed", Value(aShape.closed));

        std::vector<Vec2> positions;
        positions.reserve(aShape.points.size());
        ValueArray handles;
        bool hasHandles = false;
        for (const auto& point : aShape.points)
        {
            positions.push_back(point.position);
            handles.push_back(Value(ValueArray{Value(point.handleIn.x), Value(point.handleIn.y), Value(point.handleOut.x), Value(point.handleOut.y)}));
            hasHandles = hasHandles || point.handleIn != Vec2{} || point.handleOut != Vec2{};
        }
        aProps.set("points", writeVec2List(positions));
        if (hasHandles || aShape.kind == PathKind::Bezier)
        {
            aProps.set("handles", Value(std::move(handles)));
        }
        else
        {
            aProps.erase("handles");
        }
    }
}
