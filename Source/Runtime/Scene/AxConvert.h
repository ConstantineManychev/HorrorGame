#pragma once

#include "Core/Base/Math.h"

#include "axmol.h"

namespace hg
{
    inline ax::Vec2 toAx(const Vec2& aVector)
    {
        return {aVector.x, aVector.y};
    }

    inline Vec2 fromAx(const ax::Vec2& aVector)
    {
        return {aVector.x, aVector.y};
    }

    inline ax::Color3B toAx3B(const Color& aColor)
    {
        return ax::Color3B(aColor.r, aColor.g, aColor.b);
    }

    inline ax::Color4B toAx4B(const Color& aColor)
    {
        return ax::Color4B(aColor.r, aColor.g, aColor.b, aColor.a);
    }

    inline ax::Color4F toAx4F(const Color& aColor, float aAlpha = 1.0f)
    {
        return ax::Color4F(aColor.r / 255.0f, aColor.g / 255.0f, aColor.b / 255.0f, aColor.a / 255.0f * aAlpha);
    }
}
