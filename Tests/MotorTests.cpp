#include "doctest.h"

#include "Core/Gameplay/CharacterMotor.h"

using namespace hg;

namespace
{
    constexpr float kStep = 1.0f / 60.0f;

    const Rect kWorld{{0.0f, 0.0f}, {2000.0f, 1000.0f}};

    void simulate(MotorState& aState, const MotorInput& aInput, ControlMode aMode, const std::vector<SolidBox>& aSolids, int aFrames)
    {
        const MotorParams params;
        const MotorBody body;
        for (int frame = 0; frame < aFrames; ++frame)
        {
            stepMotor(aState, params, body, aInput, aMode, aSolids, kWorld, kStep);
        }
    }
}

TEST_SUITE("CharacterMotor")
{
    TEST_CASE("side scroll falls and lands on a platform")
    {
        MotorState state;
        state.position = {500.0f, 600.0f};
        std::vector<SolidBox> solids{{{{300.0f, 380.0f}, {400.0f, 20.0f}}, false}};

        simulate(state, {}, ControlMode::SideScroll, solids, 120);
        CHECK(state.grounded);
        CHECK(state.position.y == doctest::Approx(400.0f).epsilon(0.001));
        CHECK(state.velocity.y == doctest::Approx(0.0f));
    }

    TEST_CASE("jump leaves the ground and comes back")
    {
        MotorState state;
        state.position = {500.0f, 0.0f};
        simulate(state, {}, ControlMode::SideScroll, {}, 2);
        REQUIRE(state.grounded);

        MotorInput jump;
        jump.jumpPressed = true;
        jump.jumpHeld = true;
        simulate(state, jump, ControlMode::SideScroll, {}, 1);
        CHECK_FALSE(state.grounded);
        CHECK(state.velocity.y > 0.0f);

        MotorInput hold;
        hold.jumpHeld = true;
        simulate(state, hold, ControlMode::SideScroll, {}, 10);
        CHECK(state.position.y > 100.0f);

        simulate(state, {}, ControlMode::SideScroll, {}, 200);
        CHECK(state.grounded);
        CHECK(state.position.y == doctest::Approx(0.0f));
    }

    TEST_CASE("one way platforms are passable from below only")
    {
        MotorState state;
        state.position = {500.0f, 0.0f};
        std::vector<SolidBox> solids{{{{400.0f, 180.0f}, {300.0f, 20.0f}}, true}};

        MotorInput jump;
        jump.jumpPressed = true;
        jump.jumpHeld = true;
        simulate(state, jump, ControlMode::SideScroll, solids, 1);
        MotorInput hold;
        hold.jumpHeld = true;
        simulate(state, hold, ControlMode::SideScroll, solids, 30);
        simulate(state, {}, ControlMode::SideScroll, solids, 200);
        CHECK(state.grounded);
        CHECK(state.position.y == doctest::Approx(200.0f).epsilon(0.001));
    }

    TEST_CASE("walls stop horizontal movement")
    {
        MotorState state;
        state.position = {500.0f, 0.0f};
        std::vector<SolidBox> solids{{{{700.0f, 0.0f}, {50.0f, 500.0f}}, false}};

        MotorInput right;
        right.move = {1.0f, 0.0f};
        simulate(state, right, ControlMode::SideScroll, solids, 120);
        CHECK(state.position.x == doctest::Approx(660.0f).epsilon(0.001));
        CHECK(state.facing == 1);
    }

    TEST_CASE("top down moves in both axes without gravity and stays in world")
    {
        MotorState state;
        state.position = {500.0f, 500.0f};

        MotorInput diagonal;
        diagonal.move = {1.0f, 1.0f};
        simulate(state, diagonal, ControlMode::TopDown, {}, 30);
        CHECK(state.position.x > 500.0f);
        CHECK(state.position.y > 500.0f);
        CHECK(state.velocity.length() <= MotorParams{}.walkSpeed + 0.01f);

        simulate(state, diagonal, ControlMode::TopDown, {}, 600);
        CHECK(state.position.x == doctest::Approx(1960.0f).epsilon(0.001));
        CHECK(state.position.y == doctest::Approx(840.0f).epsilon(0.001));
    }

    TEST_CASE("control mode names")
    {
        CHECK(parseControlMode("top_down") == ControlMode::TopDown);
        CHECK(controlModeName(ControlMode::SideScroll) == "side_scroll");
        CHECK_FALSE(parseControlMode("flying").has_value());
    }
}
