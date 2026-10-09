#pragma once

#include "Core/Base/Math.h"

namespace hg
{
    struct CameraState
    {
        Vec2 center;
        float zoom = 1.0f;
        float rotation = 0.0f;
        float shake = 0.0f;
        float shakeFrequency = 18.0f;
        Vec2 shakeOffset;
    };
}
