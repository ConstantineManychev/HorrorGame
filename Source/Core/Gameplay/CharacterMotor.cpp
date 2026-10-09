#include "Core/Gameplay/CharacterMotor.h"

#include "Core/Base/ValueConvert.h"

#include <cmath>

namespace hg
{
    namespace
    {
        constexpr float kSkin = 0.01f;
        constexpr float kJumpCutFactor = 0.5f;
        constexpr int kMaxSubsteps = 8;
        constexpr float kMaxStepDistance = 24.0f;

        float approach(float aCurrent, float aTarget, float aStep)
        {
            if (aCurrent < aTarget)
            {
                return std::min(aCurrent + aStep, aTarget);
            }
            return std::max(aCurrent - aStep, aTarget);
        }

        bool overlapsHorizontally(const Rect& aA, const Rect& aB)
        {
            return aA.minX() < aB.maxX() - kSkin && aA.maxX() > aB.minX() + kSkin;
        }

        bool overlapsVertically(const Rect& aA, const Rect& aB)
        {
            return aA.minY() < aB.maxY() - kSkin && aA.maxY() > aB.minY() + kSkin;
        }

        bool isSolidFor(const SolidBox& aSolid, ControlMode aMode)
        {
            return !aSolid.oneWay || aMode == ControlMode::SideScroll;
        }

        void moveHorizontally(MotorState& aState, const MotorBody& aBody, float aDistance, ControlMode aMode, const std::vector<SolidBox>& aSolids, const Rect& aWorld)
        {
            Rect bounds = aBody.boundsAt(aState.position);
            float target = bounds.origin.x + aDistance;

            for (const auto& solid : aSolids)
            {
                if (solid.oneWay || !isSolidFor(solid, aMode) || !overlapsVertically(bounds, solid.rect))
                {
                    continue;
                }
                if (aDistance > 0.0f && bounds.maxX() <= solid.rect.minX() + kSkin)
                {
                    target = std::min(target, solid.rect.minX() - bounds.size.x);
                }
                else if (aDistance < 0.0f && bounds.minX() >= solid.rect.maxX() - kSkin)
                {
                    target = std::max(target, solid.rect.maxX());
                }
            }

            target = std::clamp(target, aWorld.minX(), std::max(aWorld.minX(), aWorld.maxX() - bounds.size.x));
            if (std::abs(target - (bounds.origin.x + aDistance)) > kSkin)
            {
                aState.velocity.x = 0.0f;
            }
            aState.position.x += target - bounds.origin.x;
        }

        void moveVertically(MotorState& aState, const MotorBody& aBody, float aDistance, ControlMode aMode, const std::vector<SolidBox>& aSolids, const Rect& aWorld)
        {
            Rect bounds = aBody.boundsAt(aState.position);
            float target = bounds.origin.y + aDistance;
            bool landed = false;
            bool blocked = false;

            for (const auto& solid : aSolids)
            {
                if (!isSolidFor(solid, aMode) || !overlapsHorizontally(bounds, solid.rect))
                {
                    continue;
                }
                if (aDistance < 0.0f && bounds.minY() >= solid.rect.maxY() - kSkin)
                {
                    if (solid.rect.maxY() > target)
                    {
                        target = solid.rect.maxY();
                        landed = true;
                    }
                }
                else if (aDistance > 0.0f && !solid.oneWay && bounds.maxY() <= solid.rect.minY() + kSkin)
                {
                    if (solid.rect.minY() - bounds.size.y < target)
                    {
                        target = solid.rect.minY() - bounds.size.y;
                        blocked = true;
                    }
                }
            }

            if (target <= aWorld.minY())
            {
                target = aWorld.minY();
                landed = aDistance <= 0.0f;
            }
            const float top = aWorld.maxY() - bounds.size.y;
            if (target >= top)
            {
                target = top;
                blocked = aDistance > 0.0f;
            }

            aState.position.y += target - bounds.origin.y;
            if (landed || blocked)
            {
                aState.velocity.y = 0.0f;
            }
            if (aMode == ControlMode::SideScroll && aDistance <= 0.0f)
            {
                aState.grounded = landed;
            }
        }

        void probeGround(MotorState& aState, const MotorBody& aBody, const std::vector<SolidBox>& aSolids, const Rect& aWorld)
        {
            const Rect bounds = aBody.boundsAt(aState.position);
            if (bounds.minY() <= aWorld.minY() + kSkin)
            {
                aState.grounded = true;
                return;
            }
            for (const auto& solid : aSolids)
            {
                if (overlapsHorizontally(bounds, solid.rect) && std::abs(bounds.minY() - solid.rect.maxY()) <= kSkin * 2.0f)
                {
                    aState.grounded = true;
                    return;
                }
            }
            aState.grounded = false;
        }
    }

    std::string_view controlModeName(ControlMode aMode)
    {
        switch (aMode)
        {
        case ControlMode::None:
            return "none";
        case ControlMode::SideScroll:
            return "side_scroll";
        case ControlMode::TopDown:
            return "top_down";
        }
        return "none";
    }

    std::optional<ControlMode> parseControlMode(std::string_view aName)
    {
        if (aName == "none")
        {
            return ControlMode::None;
        }
        if (aName == "side_scroll")
        {
            return ControlMode::SideScroll;
        }
        if (aName == "top_down")
        {
            return ControlMode::TopDown;
        }
        return std::nullopt;
    }

    MotorParams MotorParams::fromProps(const ValueObject& aProps)
    {
        MotorParams params;
        params.runSpeed = aProps.get("run_speed").asFloat(params.runSpeed);
        params.acceleration = aProps.get("acceleration").asFloat(params.acceleration);
        params.jumpSpeed = aProps.get("jump_speed").asFloat(params.jumpSpeed);
        params.gravity = aProps.get("gravity").asFloat(params.gravity);
        params.maxFallSpeed = aProps.get("max_fall_speed").asFloat(params.maxFallSpeed);
        params.coyoteTime = aProps.get("coyote_time").asFloat(params.coyoteTime);
        params.jumpBuffer = aProps.get("jump_buffer").asFloat(params.jumpBuffer);
        params.walkSpeed = aProps.get("walk_speed").asFloat(params.walkSpeed);
        params.walkAcceleration = aProps.get("walk_acceleration").asFloat(params.walkAcceleration);
        return params;
    }

    Rect MotorBody::boundsAt(const Vec2& aPosition) const
    {
        return Rect::fromCenter(aPosition + offset, size);
    }

    MotorBody MotorBody::fromProps(const ValueObject& aProps)
    {
        MotorBody body;
        body.size = readVec2(aProps.get("size"), body.size);
        body.offset = readVec2(aProps.get("offset"), body.offset);
        return body;
    }

    void stepMotor(MotorState& aState, const MotorParams& aParams, const MotorBody& aBody, const MotorInput& aInput, ControlMode aMode, const std::vector<SolidBox>& aSolids, const Rect& aWorldBounds, float aDelta)
    {
        if (aDelta <= 0.0f || aMode == ControlMode::None)
        {
            return;
        }

        if (aInput.move.x > 0.01f)
        {
            aState.facing = 1;
        }
        else if (aInput.move.x < -0.01f)
        {
            aState.facing = -1;
        }

        if (aMode == ControlMode::SideScroll)
        {
            aState.velocity.x = approach(aState.velocity.x, aInput.move.x * aParams.runSpeed, aParams.acceleration * aDelta);

            aState.coyoteTimer = aState.grounded ? aParams.coyoteTime : std::max(0.0f, aState.coyoteTimer - aDelta);
            aState.jumpBufferTimer = aInput.jumpPressed ? aParams.jumpBuffer : std::max(0.0f, aState.jumpBufferTimer - aDelta);

            if (aState.jumpBufferTimer > 0.0f && aState.coyoteTimer > 0.0f)
            {
                aState.velocity.y = aParams.jumpSpeed;
                aState.jumpBufferTimer = 0.0f;
                aState.coyoteTimer = 0.0f;
                aState.grounded = false;
            }
            if (!aInput.jumpHeld && aState.velocity.y > 0.0f)
            {
                aState.velocity.y *= std::pow(kJumpCutFactor, aDelta * 30.0f);
            }

            aState.velocity.y = std::max(aState.velocity.y - aParams.gravity * aDelta, -aParams.maxFallSpeed);
        }
        else
        {
            Vec2 desired = aInput.move;
            if (desired.length() > 1.0f)
            {
                desired = desired.normalized();
            }
            desired *= aParams.walkSpeed;
            aState.velocity.x = approach(aState.velocity.x, desired.x, aParams.walkAcceleration * aDelta);
            aState.velocity.y = approach(aState.velocity.y, desired.y, aParams.walkAcceleration * aDelta);
            aState.grounded = false;
            aState.coyoteTimer = 0.0f;
            aState.jumpBufferTimer = 0.0f;
        }

        const Vec2 travel = aState.velocity * aDelta;
        const int steps = std::clamp(static_cast<int>(std::ceil(travel.length() / kMaxStepDistance)), 1, kMaxSubsteps);
        for (int step = 0; step < steps; ++step)
        {
            moveHorizontally(aState, aBody, aState.velocity.x * aDelta / static_cast<float>(steps), aMode, aSolids, aWorldBounds);
            moveVertically(aState, aBody, aState.velocity.y * aDelta / static_cast<float>(steps), aMode, aSolids, aWorldBounds);
        }

        if (aMode == ControlMode::SideScroll)
        {
            probeGround(aState, aBody, aSolids, aWorldBounds);
        }
    }
}
