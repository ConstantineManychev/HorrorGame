#include "Runtime/Animation/TimelinePlayer.h"

#include "Core/Animation/TimelineEvaluator.h"

#include <cmath>
#include <vector>

namespace hg
{
    namespace
    {
        constexpr float kEventEpsilon = 1e-4f;
    }

    TimelinePlayer::TimelinePlayer(TimelineDesc aTimeline, TimelineTarget& aTarget)
        : mTimeline(std::move(aTimeline))
        , mTarget(aTarget)
    {
    }

    void TimelinePlayer::play(float aFrom)
    {
        mTime = std::clamp(aFrom, 0.0f, duration());
        mPlaying = true;
        mFinished = false;
        fireEvents(mTime - kEventEpsilon, mTime);
        applyAt(mTime);
    }

    void TimelinePlayer::pause()
    {
        mPlaying = false;
    }

    void TimelinePlayer::resume()
    {
        if (!mFinished)
        {
            mPlaying = true;
        }
    }

    void TimelinePlayer::stop()
    {
        mPlaying = false;
        mFinished = true;
    }

    void TimelinePlayer::seek(float aTime)
    {
        mTime = std::clamp(aTime, 0.0f, duration());
        mFinished = false;
        applyAt(mTime);
    }

    void TimelinePlayer::update(float aDelta)
    {
        if (!mPlaying)
        {
            return;
        }

        const float previous = mTime;
        mTime += aDelta * mSpeed;
        const float end = duration();

        if (mTime >= end)
        {
            fireEvents(previous, end);
            if (mTimeline.loop && end > 0.0f)
            {
                mTime = std::fmod(mTime, end);
                fireEvents(-kEventEpsilon, mTime);
            }
            else
            {
                mTime = end;
                applyAt(mTime);
                mPlaying = false;
                mFinished = true;
                return;
            }
        }
        else
        {
            fireEvents(previous, mTime);
        }
        applyAt(mTime);
    }

    void TimelinePlayer::setSpeed(float aSpeed)
    {
        mSpeed = aSpeed;
    }

    float TimelinePlayer::speed() const
    {
        return mSpeed;
    }

    float TimelinePlayer::time() const
    {
        return mTime;
    }

    float TimelinePlayer::duration() const
    {
        return std::max(0.0f, mTimeline.duration);
    }

    bool TimelinePlayer::isPlaying() const
    {
        return mPlaying;
    }

    bool TimelinePlayer::isFinished() const
    {
        return mFinished;
    }

    const TimelineDesc& TimelinePlayer::timeline() const
    {
        return mTimeline;
    }

    void TimelinePlayer::applyAt(float aTime)
    {
        for (const auto& track : mTimeline.tracks)
        {
            if (track.muted)
            {
                continue;
            }
            switch (track.kind)
            {
            case TrackKind::Property:
            {
                auto value = sampleKeyframes(track.keys, aTime);
                if (!value)
                {
                    break;
                }
                if (track.target.isCamera())
                {
                    mTarget.applyCameraProperty(track.property, *value);
                }
                else if (track.target.isObject())
                {
                    mTarget.applyObjectProperty(track.target.uid, track.property, *value);
                }
                break;
            }
            case TrackKind::Path:
            {
                auto progress = samplePathProgress(track, aTime);
                const PathSampler* sampler = progress ? mTarget.worldPath(track.path) : nullptr;
                if (!sampler || sampler->empty())
                {
                    break;
                }
                const PathSample sample = sampler->sampleNormalized(*progress);
                if (track.target.isCamera())
                {
                    mTarget.applyCameraProperty("center", Value(ValueArray{Value(sample.position.x), Value(sample.position.y)}));
                }
                else if (track.target.isObject())
                {
                    std::optional<float> rotation;
                    if (track.orient)
                    {
                        rotation = -radiansToDegrees(sample.tangent.angle());
                    }
                    mTarget.placeObjectOnPath(track.target.uid, sample.position, rotation);
                }
                break;
            }
            case TrackKind::Event:
                break;
            }
        }
    }

    void TimelinePlayer::fireEvents(float aFromExclusive, float aToInclusive)
    {
        std::vector<const TimelineEvent*> events;
        for (const auto& track : mTimeline.tracks)
        {
            collectEvents(track, aFromExclusive, aToInclusive, events);
        }
        std::stable_sort(events.begin(), events.end(), [](const TimelineEvent* aLeft, const TimelineEvent* aRight)
        {
            return aLeft->time < aRight->time;
        });
        for (const TimelineEvent* event : events)
        {
            mTarget.runTimelineEvent(event->action);
        }
    }
}
