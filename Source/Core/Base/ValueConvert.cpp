#include "Core/Base/ValueConvert.h"

#include <algorithm>
#include <cctype>

namespace hg
{
    namespace
    {
        uint8_t toByte(const Value& aValue)
        {
            return static_cast<uint8_t>(std::clamp<int64_t>(aValue.asInt(), 0, 255));
        }

        std::optional<uint8_t> parseHexByte(std::string_view aText)
        {
            if (aText.size() != 2)
            {
                return std::nullopt;
            }
            int result = 0;
            for (char character : aText)
            {
                result <<= 4;
                if (character >= '0' && character <= '9')
                {
                    result |= character - '0';
                }
                else if (character >= 'a' && character <= 'f')
                {
                    result |= character - 'a' + 10;
                }
                else if (character >= 'A' && character <= 'F')
                {
                    result |= character - 'A' + 10;
                }
                else
                {
                    return std::nullopt;
                }
            }
            return static_cast<uint8_t>(result);
        }
    }

    std::optional<Vec2> readVec2(const Value& aValue)
    {
        if (const ValueArray* array = aValue.getArray())
        {
            if (array->size() == 2 && (*array)[0].isNumber() && (*array)[1].isNumber())
            {
                return Vec2{(*array)[0].asFloat(), (*array)[1].asFloat()};
            }
            return std::nullopt;
        }
        if (const ValueObject* object = aValue.getObject())
        {
            const Value& x = object->get("x");
            const Value& y = object->get("y");
            if (x.isNumber() && y.isNumber())
            {
                return Vec2{x.asFloat(), y.asFloat()};
            }
            return std::nullopt;
        }
        if (aValue.isNumber())
        {
            return Vec2{aValue.asFloat(), aValue.asFloat()};
        }
        return std::nullopt;
    }

    Vec2 readVec2(const Value& aValue, const Vec2& aFallback)
    {
        return readVec2(aValue).value_or(aFallback);
    }

    Value writeVec2(const Vec2& aVector)
    {
        return Value(ValueArray{Value(aVector.x), Value(aVector.y)});
    }

    std::optional<Color> readColor(const Value& aValue)
    {
        if (const ValueArray* array = aValue.getArray())
        {
            if (array->size() != 3 && array->size() != 4)
            {
                return std::nullopt;
            }
            for (const auto& component : *array)
            {
                if (!component.isNumber())
                {
                    return std::nullopt;
                }
            }
            Color color;
            color.r = toByte((*array)[0]);
            color.g = toByte((*array)[1]);
            color.b = toByte((*array)[2]);
            color.a = array->size() == 4 ? toByte((*array)[3]) : 255;
            return color;
        }
        if (aValue.isString())
        {
            std::string_view text = aValue.asString();
            if (!text.empty() && text.front() == '#')
            {
                text.remove_prefix(1);
            }
            if (text.size() != 6 && text.size() != 8)
            {
                return std::nullopt;
            }
            auto r = parseHexByte(text.substr(0, 2));
            auto g = parseHexByte(text.substr(2, 2));
            auto b = parseHexByte(text.substr(4, 2));
            auto a = text.size() == 8 ? parseHexByte(text.substr(6, 2)) : std::optional<uint8_t>(255);
            if (!r || !g || !b || !a)
            {
                return std::nullopt;
            }
            return Color{*r, *g, *b, *a};
        }
        return std::nullopt;
    }

    Color readColor(const Value& aValue, const Color& aFallback)
    {
        return readColor(aValue).value_or(aFallback);
    }

    Value writeColor(const Color& aColor)
    {
        if (aColor.a == 255)
        {
            return Value(ValueArray{Value(aColor.r), Value(aColor.g), Value(aColor.b)});
        }
        return Value(ValueArray{Value(aColor.r), Value(aColor.g), Value(aColor.b), Value(aColor.a)});
    }

    std::optional<Rect> readRect(const Value& aValue)
    {
        const ValueArray* array = aValue.getArray();
        if (!array || array->size() != 4)
        {
            return std::nullopt;
        }
        for (const auto& component : *array)
        {
            if (!component.isNumber())
            {
                return std::nullopt;
            }
        }
        return Rect{{(*array)[0].asFloat(), (*array)[1].asFloat()}, {(*array)[2].asFloat(), (*array)[3].asFloat()}};
    }

    Value writeRect(const Rect& aRect)
    {
        return Value(ValueArray{Value(aRect.origin.x), Value(aRect.origin.y), Value(aRect.size.x), Value(aRect.size.y)});
    }

    std::vector<Vec2> readVec2List(const Value& aValue)
    {
        std::vector<Vec2> points;
        for (const auto& element : aValue.asArray())
        {
            if (auto point = readVec2(element))
            {
                points.push_back(*point);
            }
        }
        return points;
    }

    Value writeVec2List(const std::vector<Vec2>& aPoints)
    {
        ValueArray array;
        array.reserve(aPoints.size());
        for (const auto& point : aPoints)
        {
            array.push_back(writeVec2(point));
        }
        return Value(std::move(array));
    }

    std::vector<std::string> readStringList(const Value& aValue)
    {
        std::vector<std::string> strings;
        for (const auto& element : aValue.asArray())
        {
            if (element.isString())
            {
                strings.push_back(element.asString());
            }
        }
        return strings;
    }

    Value writeStringList(const std::vector<std::string>& aStrings)
    {
        ValueArray array;
        array.reserve(aStrings.size());
        for (const auto& text : aStrings)
        {
            array.emplace_back(text);
        }
        return Value(std::move(array));
    }
}
