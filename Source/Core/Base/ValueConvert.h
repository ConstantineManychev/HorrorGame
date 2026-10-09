#pragma once

#include "Core/Base/Math.h"
#include "Core/Base/Value.h"

#include <optional>
#include <string>
#include <vector>

namespace hg
{
    std::optional<Vec2> readVec2(const Value& aValue);
    Vec2 readVec2(const Value& aValue, const Vec2& aFallback);
    Value writeVec2(const Vec2& aVector);

    std::optional<Color> readColor(const Value& aValue);
    Color readColor(const Value& aValue, const Color& aFallback);
    Value writeColor(const Color& aColor);

    std::optional<Rect> readRect(const Value& aValue);
    Value writeRect(const Rect& aRect);

    std::vector<Vec2> readVec2List(const Value& aValue);
    Value writeVec2List(const std::vector<Vec2>& aPoints);

    std::vector<std::string> readStringList(const Value& aValue);
    Value writeStringList(const std::vector<std::string>& aStrings);
}
