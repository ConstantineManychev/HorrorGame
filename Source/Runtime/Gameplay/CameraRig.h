#pragma once

#include "Core/Base/Value.h"
#include "Runtime/Scene/CameraState.h"

#include <functional>
#include <optional>
#include <string>

namespace hg
{
    struct CameraRigSettings
    {
        Vec2 worldSize{3840.0f, 2160.0f};
        Vec2 deadZone{0.15f, 0.2f};
        float damping = 6.0f;
        float blendOutTime = 0.6f;
    };

    class CameraRig
    {
    public:
        void configure(const CameraRigSettings& aSettings);
        void setVisibleSizeProvider(std::function<Vec2(float)> aProvider);

        void setTarget(std::optional<Vec2> aTarget);
        void snapToTarget();
        void reset(const Vec2& aCenter);

        void beginFrame();
        void drive(const std::string& aProperty, const Value& aValue);
        bool isDriven() const;
        void update(float aDelta);

        const CameraState& output() const;
        const CameraState& follow() const;

    private:
        struct Channel
        {
            float weight = 0.0f;
            bool drivenThisFrame = false;
        };

        Vec2 clampToWorld(const Vec2& aCenter, float aZoom) const;
        Vec2 visibleSize(float aZoom) const;

        CameraRigSettings mSettings;
        std::function<Vec2(float)> mVisibleSize;
        std::optional<Vec2> mTarget;
        CameraState mFollow;
        CameraState mDriven;
        CameraState mOutput;
        Channel mCenter;
        Channel mZoom;
        Channel mRotation;
        Channel mShake;
        float mTime = 0.0f;
    };
}
