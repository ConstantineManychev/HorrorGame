#pragma once

#include "Core/Animation/Path.h"
#include "Core/Animation/Timeline.h"

#include <optional>
#include <string>

namespace hg
{
    class TimelineTarget
    {
    public:
        virtual ~TimelineTarget() = default;

        virtual void applyObjectProperty(ObjectId aUid, const std::string& aProperty, const Value& aValue) = 0;
        virtual void applyCameraProperty(const std::string& aProperty, const Value& aValue) = 0;
        virtual void placeObjectOnPath(ObjectId aUid, const Vec2& aWorldPosition, std::optional<float> aWorldRotation) = 0;
        virtual const PathSampler* worldPath(ObjectId aPathUid) = 0;
        virtual void runTimelineEvent(const ValueObject& aAction) = 0;
    };

    class TimelinePlayer
    {
    public:
        TimelinePlayer(TimelineDesc aTimeline, TimelineTarget& aTarget);

        void play(float aFrom = 0.0f);
        void pause();
        void resume();
        void stop();
        void seek(float aTime);
        void update(float aDelta);

        void setSpeed(float aSpeed);
        float speed() const;
        float time() const;
        float duration() const;
        bool isPlaying() const;
        bool isFinished() const;
        const TimelineDesc& timeline() const;

        void applyAt(float aTime);

    private:
        void fireEvents(float aFromExclusive, float aToInclusive);

        TimelineDesc mTimeline;
        TimelineTarget& mTarget;
        float mTime = 0.0f;
        float mSpeed = 1.0f;
        bool mPlaying = false;
        bool mFinished = false;
    };
}
