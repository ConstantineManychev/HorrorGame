#include "Core/Animation/Easing.h"

#include "Core/Base/Math.h"

#include <cmath>

namespace hg
{
    namespace
    {
        struct EaseName
        {
            EaseType type;
            std::string_view name;
        };

        constexpr std::array<EaseName, 20> kEaseNames{{
            {EaseType::Linear, "linear"},
            {EaseType::Step, "step"},
            {EaseType::SineIn, "sineIn"},
            {EaseType::SineOut, "sineOut"},
            {EaseType::SineInOut, "sineInOut"},
            {EaseType::QuadIn, "quadIn"},
            {EaseType::QuadOut, "quadOut"},
            {EaseType::QuadInOut, "quadInOut"},
            {EaseType::CubicIn, "cubicIn"},
            {EaseType::CubicOut, "cubicOut"},
            {EaseType::CubicInOut, "cubicInOut"},
            {EaseType::ExpoIn, "expoIn"},
            {EaseType::ExpoOut, "expoOut"},
            {EaseType::ExpoInOut, "expoInOut"},
            {EaseType::BackIn, "backIn"},
            {EaseType::BackOut, "backOut"},
            {EaseType::BackInOut, "backInOut"},
            {EaseType::ElasticOut, "elasticOut"},
            {EaseType::BounceOut, "bounceOut"},
            {EaseType::Bezier, "bezier"},
        }};

        constexpr std::array<EaseType, 20> kAllEaseTypes{
            EaseType::Linear,
            EaseType::Step,
            EaseType::SineIn,
            EaseType::SineOut,
            EaseType::SineInOut,
            EaseType::QuadIn,
            EaseType::QuadOut,
            EaseType::QuadInOut,
            EaseType::CubicIn,
            EaseType::CubicOut,
            EaseType::CubicInOut,
            EaseType::ExpoIn,
            EaseType::ExpoOut,
            EaseType::ExpoInOut,
            EaseType::BackIn,
            EaseType::BackOut,
            EaseType::BackInOut,
            EaseType::ElasticOut,
            EaseType::BounceOut,
            EaseType::Bezier,
        };

        constexpr float kBackOvershoot = 1.70158f;

        float bounceOut(float aT)
        {
            constexpr float n = 7.5625f;
            constexpr float d = 2.75f;
            if (aT < 1.0f / d)
            {
                return n * aT * aT;
            }
            if (aT < 2.0f / d)
            {
                aT -= 1.5f / d;
                return n * aT * aT + 0.75f;
            }
            if (aT < 2.5f / d)
            {
                aT -= 2.25f / d;
                return n * aT * aT + 0.9375f;
            }
            aT -= 2.625f / d;
            return n * aT * aT + 0.984375f;
        }

        float bezierComponent(float aT, float aP1, float aP2)
        {
            const float inverse = 1.0f - aT;
            return 3.0f * inverse * inverse * aT * aP1 + 3.0f * inverse * aT * aT * aP2 + aT * aT * aT;
        }

        float bezierComponentDerivative(float aT, float aP1, float aP2)
        {
            const float inverse = 1.0f - aT;
            return 3.0f * inverse * inverse * aP1 + 6.0f * inverse * aT * (aP2 - aP1) + 3.0f * aT * aT * (1.0f - aP2);
        }
    }

    float evaluateCubicBezier(const std::array<float, 4>& aControl, float aX)
    {
        const float x1 = clamp01(aControl[0]);
        const float y1 = aControl[1];
        const float x2 = clamp01(aControl[2]);
        const float y2 = aControl[3];

        float t = aX;
        for (int iteration = 0; iteration < 8; ++iteration)
        {
            const float error = bezierComponent(t, x1, x2) - aX;
            if (std::abs(error) < 1e-6f)
            {
                return bezierComponent(t, y1, y2);
            }
            const float derivative = bezierComponentDerivative(t, x1, x2);
            if (std::abs(derivative) < 1e-6f)
            {
                break;
            }
            t -= error / derivative;
        }

        float low = 0.0f;
        float high = 1.0f;
        t = aX;
        for (int iteration = 0; iteration < 32; ++iteration)
        {
            const float x = bezierComponent(t, x1, x2);
            if (std::abs(x - aX) < 1e-6f)
            {
                break;
            }
            if (x < aX)
            {
                low = t;
            }
            else
            {
                high = t;
            }
            t = (low + high) * 0.5f;
        }
        return bezierComponent(t, y1, y2);
    }

    float Easing::apply(float aT) const
    {
        const float t = clamp01(aT);
        switch (type)
        {
        case EaseType::Linear:
            return t;
        case EaseType::Step:
            return t >= 1.0f ? 1.0f : 0.0f;
        case EaseType::SineIn:
            return 1.0f - std::cos(t * kPi * 0.5f);
        case EaseType::SineOut:
            return std::sin(t * kPi * 0.5f);
        case EaseType::SineInOut:
            return -(std::cos(kPi * t) - 1.0f) * 0.5f;
        case EaseType::QuadIn:
            return t * t;
        case EaseType::QuadOut:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case EaseType::QuadInOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
        case EaseType::CubicIn:
            return t * t * t;
        case EaseType::CubicOut:
            return 1.0f - std::pow(1.0f - t, 3.0f);
        case EaseType::CubicInOut:
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
        case EaseType::ExpoIn:
            return t <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
        case EaseType::ExpoOut:
            return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
        case EaseType::ExpoInOut:
            if (t <= 0.0f || t >= 1.0f)
            {
                return t;
            }
            return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
        case EaseType::BackIn:
            return (kBackOvershoot + 1.0f) * t * t * t - kBackOvershoot * t * t;
        case EaseType::BackOut:
        {
            const float u = t - 1.0f;
            return 1.0f + (kBackOvershoot + 1.0f) * u * u * u + kBackOvershoot * u * u;
        }
        case EaseType::BackInOut:
        {
            const float c = kBackOvershoot * 1.525f;
            if (t < 0.5f)
            {
                return (std::pow(2.0f * t, 2.0f) * ((c + 1.0f) * 2.0f * t - c)) * 0.5f;
            }
            return (std::pow(2.0f * t - 2.0f, 2.0f) * ((c + 1.0f) * (t * 2.0f - 2.0f) + c) + 2.0f) * 0.5f;
        }
        case EaseType::ElasticOut:
        {
            if (t <= 0.0f || t >= 1.0f)
            {
                return t;
            }
            constexpr float c = (2.0f * kPi) / 3.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c) + 1.0f;
        }
        case EaseType::BounceOut:
            return bounceOut(t);
        case EaseType::Bezier:
            return evaluateCubicBezier(bezier, t);
        }
        return t;
    }

    Easing Easing::of(EaseType aType)
    {
        Easing easing;
        easing.type = aType;
        return easing;
    }

    std::string_view easeTypeName(EaseType aType)
    {
        for (const auto& entry : kEaseNames)
        {
            if (entry.type == aType)
            {
                return entry.name;
            }
        }
        return "linear";
    }

    std::optional<EaseType> parseEaseType(std::string_view aName)
    {
        for (const auto& entry : kEaseNames)
        {
            if (entry.name == aName)
            {
                return entry.type;
            }
        }
        return std::nullopt;
    }

    std::span<const EaseType> allEaseTypes()
    {
        return kAllEaseTypes;
    }

    Easing readEasing(const Value& aValue)
    {
        Easing easing;
        if (aValue.isString())
        {
            easing.type = parseEaseType(aValue.asString()).value_or(EaseType::Linear);
            return easing;
        }
        if (const ValueObject* object = aValue.getObject())
        {
            const ValueArray& control = object->get("bezier").asArray();
            if (control.size() == 4)
            {
                easing.type = EaseType::Bezier;
                for (size_t index = 0; index < 4; ++index)
                {
                    easing.bezier[index] = control[index].asFloat();
                }
            }
        }
        return easing;
    }

    Value writeEasing(const Easing& aEasing)
    {
        if (aEasing.type == EaseType::Bezier)
        {
            ValueArray control;
            for (float component : aEasing.bezier)
            {
                control.emplace_back(component);
            }
            return Value(ValueObject{{"bezier", Value(std::move(control))}});
        }
        return Value(easeTypeName(aEasing.type));
    }
}
