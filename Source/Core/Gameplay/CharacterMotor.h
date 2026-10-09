#pragma once

#include "Core/Base/Math.h"
#include "Core/Base/Value.h"

#include <optional>
#include <string_view>
#include <vector>

namespace hg
{
    enum class ControlMode
    {
        None,
        SideScroll,
        TopDown
    };

    std::string_view controlModeName(ControlMode aMode);
    std::optional<ControlMode> parseControlMode(std::string_view aName);

    struct SolidBox
    {
        Rect rect;
        bool oneWay = false;
    };

    struct MotorParams
    {
        float runSpeed = 520.0f;
        float acceleration = 4000.0f;
        float jumpSpeed = 1250.0f;
        float gravity = 3600.0f;
        float maxFallSpeed = 2200.0f;
        float coyoteTime = 0.1f;
        float jumpBuffer = 0.12f;
        float walkSpeed = 420.0f;
        float walkAcceleration = 3200.0f;

        static MotorParams fromProps(const ValueObject& aProps);
    };

    struct MotorBody
    {
        Vec2 size{80.0f, 160.0f};
        Vec2 offset{0.0f, 80.0f};

        Rect boundsAt(const Vec2& aPosition) const;
        static MotorBody fromProps(const ValueObject& aProps);
    };

    struct MotorInput
    {
        Vec2 move;
        bool jumpPressed = false;
        bool jumpHeld = false;
    };

    struct MotorState
    {
        Vec2 position;
        Vec2 velocity;
        bool grounded = false;
        float coyoteTimer = 0.0f;
        float jumpBufferTimer = 0.0f;
        int facing = 1;
    };

    void stepMotor(MotorState& aState, const MotorParams& aParams, const MotorBody& aBody, const MotorInput& aInput, ControlMode aMode, const std::vector<SolidBox>& aSolids, const Rect& aWorldBounds, float aDelta);
}
