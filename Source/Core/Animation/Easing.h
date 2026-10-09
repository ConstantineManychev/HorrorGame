#pragma once

#include "Core/Base/Value.h"

#include <array>
#include <optional>
#include <span>
#include <string_view>

namespace hg
{
    enum class EaseType
    {
        Linear,
        Step,
        SineIn,
        SineOut,
        SineInOut,
        QuadIn,
        QuadOut,
        QuadInOut,
        CubicIn,
        CubicOut,
        CubicInOut,
        ExpoIn,
        ExpoOut,
        ExpoInOut,
        BackIn,
        BackOut,
        BackInOut,
        ElasticOut,
        BounceOut,
        Bezier
    };

    struct Easing
    {
        EaseType type = EaseType::Linear;
        std::array<float, 4> bezier{0.25f, 0.1f, 0.25f, 1.0f};

        float apply(float aT) const;

        bool operator==(const Easing& aOther) const = default;

        static Easing of(EaseType aType);
    };

    float evaluateCubicBezier(const std::array<float, 4>& aControl, float aX);

    std::string_view easeTypeName(EaseType aType);
    std::optional<EaseType> parseEaseType(std::string_view aName);
    std::span<const EaseType> allEaseTypes();

    Easing readEasing(const Value& aValue);
    Value writeEasing(const Easing& aEasing);
}
