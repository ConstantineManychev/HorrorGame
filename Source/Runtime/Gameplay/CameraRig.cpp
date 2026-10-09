#include "Runtime/Gameplay/CameraRig.h"

#include "Core/Base/ValueConvert.h"

#include <cmath>

namespace hg
{
    namespace
    {
        float blendChannel(float aFollow, float aDriven, float aWeight)
        {
            return lerp(aFollow, aDriven, aWeight);
        }

        void decay(float& aWeight, bool aDriven, float aDelta, float aBlendTime)
        {
            if (aDriven)
            {
                aWeight = 1.0f;
            }
            else if (aBlendTime <= 0.0f)
            {
                aWeight = 0.0f;
            }
            else
            {
                aWeight = std::max(0.0f, aWeight - aDelta / aBlendTime);
            }
        }
    }

    void CameraRig::configure(const CameraRigSettings& aSettings)
    {
        mSettings = aSettings;
    }

    void CameraRig::setVisibleSizeProvider(std::function<Vec2(float)> aProvider)
    {
        mVisibleSize = std::move(aProvider);
    }

    void CameraRig::setTarget(std::optional<Vec2> aTarget)
    {
        mTarget = aTarget;
    }

    void CameraRig::snapToTarget()
    {
        if (mTarget)
        {
            mFollow.center = clampToWorld(*mTarget, mFollow.zoom);
        }
        mOutput = mFollow;
    }

    void CameraRig::reset(const Vec2& aCenter)
    {
        mFollow = CameraState{};
        mFollow.center = clampToWorld(aCenter, 1.0f);
        mDriven = mFollow;
        mOutput = mFollow;
        mCenter = {};
        mZoom = {};
        mRotation = {};
        mShake = {};
    }

    void CameraRig::beginFrame()
    {
        mCenter.drivenThisFrame = false;
        mZoom.drivenThisFrame = false;
        mRotation.drivenThisFrame = false;
        mShake.drivenThisFrame = false;
    }

    void CameraRig::drive(const std::string& aProperty, const Value& aValue)
    {
        if (aProperty == "center")
        {
            mDriven.center = readVec2(aValue, mDriven.center);
            mCenter.drivenThisFrame = true;
        }
        else if (aProperty == "zoom")
        {
            mDriven.zoom = std::max(0.01f, aValue.asFloat(1.0f));
            mZoom.drivenThisFrame = true;
        }
        else if (aProperty == "rotation")
        {
            mDriven.rotation = aValue.asFloat();
            mRotation.drivenThisFrame = true;
        }
        else if (aProperty == "shake")
        {
            mDriven.shake = std::max(0.0f, aValue.asFloat());
            mShake.drivenThisFrame = true;
        }
        else if (aProperty == "shake_frequency")
        {
            mDriven.shakeFrequency = std::max(0.0f, aValue.asFloat(18.0f));
            mShake.drivenThisFrame = true;
        }
    }

    bool CameraRig::isDriven() const
    {
        return mCenter.weight > 0.0f || mZoom.weight > 0.0f || mRotation.weight > 0.0f || mShake.weight > 0.0f;
    }

    void CameraRig::update(float aDelta)
    {
        mTime += aDelta;

        if (mTarget)
        {
            const Vec2 visible = visibleSize(mFollow.zoom);
            const Vec2 halfZone{visible.x * mSettings.deadZone.x * 0.5f, visible.y * mSettings.deadZone.y * 0.5f};
            Vec2 desired = mFollow.center;
            const Vec2 delta = *mTarget - mFollow.center;
            if (delta.x > halfZone.x)
            {
                desired.x = mTarget->x - halfZone.x;
            }
            else if (delta.x < -halfZone.x)
            {
                desired.x = mTarget->x + halfZone.x;
            }
            if (delta.y > halfZone.y)
            {
                desired.y = mTarget->y - halfZone.y;
            }
            else if (delta.y < -halfZone.y)
            {
                desired.y = mTarget->y + halfZone.y;
            }

            const float blend = mSettings.damping <= 0.0f ? 1.0f : 1.0f - std::exp(-mSettings.damping * aDelta);
            mFollow.center = Vec2::lerp(mFollow.center, desired, blend);
        }
        mFollow.center = clampToWorld(mFollow.center, mFollow.zoom);

        decay(mCenter.weight, mCenter.drivenThisFrame, aDelta, mSettings.blendOutTime);
        decay(mZoom.weight, mZoom.drivenThisFrame, aDelta, mSettings.blendOutTime);
        decay(mRotation.weight, mRotation.drivenThisFrame, aDelta, mSettings.blendOutTime);
        decay(mShake.weight, mShake.drivenThisFrame, aDelta, mSettings.blendOutTime);

        if (!mCenter.drivenThisFrame && mCenter.weight <= 0.0f)
        {
            mDriven.center = mFollow.center;
        }
        if (!mZoom.drivenThisFrame && mZoom.weight <= 0.0f)
        {
            mDriven.zoom = mFollow.zoom;
        }

        mOutput.zoom = blendChannel(mFollow.zoom, mDriven.zoom, mZoom.weight);
        mOutput.center = clampToWorld(Vec2::lerp(mFollow.center, mDriven.center, mCenter.weight), mOutput.zoom);
        mOutput.rotation = blendChannel(mFollow.rotation, mDriven.rotation, mRotation.weight);
        mOutput.shake = blendChannel(mFollow.shake, mDriven.shake, mShake.weight);
        mOutput.shakeFrequency = mDriven.shakeFrequency;

        if (mOutput.shake > 0.0f)
        {
            const float phase = mTime * mOutput.shakeFrequency;
            mOutput.shakeOffset = Vec2{std::sin(phase) * 0.6f + std::sin(phase * 2.3f + 1.7f) * 0.4f, std::sin(phase * 1.3f + 0.5f) * 0.6f + std::sin(phase * 2.9f + 2.1f) * 0.4f} * mOutput.shake;
        }
        else
        {
            mOutput.shakeOffset = {};
        }
    }

    const CameraState& CameraRig::output() const
    {
        return mOutput;
    }

    const CameraState& CameraRig::follow() const
    {
        return mFollow;
    }

    Vec2 CameraRig::clampToWorld(const Vec2& aCenter, float aZoom) const
    {
        const Vec2 half = visibleSize(aZoom) * 0.5f;
        Vec2 result = aCenter;
        const Vec2 world = mSettings.worldSize;
        result.x = world.x <= half.x * 2.0f ? world.x * 0.5f : std::clamp(result.x, half.x, world.x - half.x);
        result.y = world.y <= half.y * 2.0f ? world.y * 0.5f : std::clamp(result.y, half.y, world.y - half.y);
        return result;
    }

    Vec2 CameraRig::visibleSize(float aZoom) const
    {
        return mVisibleSize ? mVisibleSize(aZoom) : mSettings.worldSize;
    }
}
