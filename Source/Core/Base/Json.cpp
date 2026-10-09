#include "Core/Base/Json.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/internal/dtoa.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace hg
{
    namespace
    {
        constexpr unsigned kParseFlags = rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseFullPrecisionFlag;
        constexpr size_t kMaxInlineArrayLength = 16;

        Value convertFromRapid(const rapidjson::Value& aSource)
        {
            if (aSource.IsNull())
            {
                return Value();
            }
            if (aSource.IsBool())
            {
                return Value(aSource.GetBool());
            }
            if (aSource.IsInt64())
            {
                return Value(aSource.GetInt64());
            }
            if (aSource.IsNumber())
            {
                return Value(aSource.GetDouble());
            }
            if (aSource.IsString())
            {
                return Value(std::string(aSource.GetString(), aSource.GetStringLength()));
            }
            if (aSource.IsArray())
            {
                ValueArray array;
                array.reserve(aSource.Size());
                for (const auto& element : aSource.GetArray())
                {
                    array.push_back(convertFromRapid(element));
                }
                return Value(std::move(array));
            }
            ValueObject object;
            for (const auto& member : aSource.GetObject())
            {
                object.set(std::string(member.name.GetString(), member.name.GetStringLength()), convertFromRapid(member.value));
            }
            return Value(std::move(object));
        }

        void describeOffset(std::string_view aText, size_t aOffset, size_t& aLine, size_t& aColumn)
        {
            aLine = 1;
            aColumn = 1;
            const size_t limit = std::min(aOffset, aText.size());
            for (size_t index = 0; index < limit; ++index)
            {
                if (aText[index] == '\n')
                {
                    ++aLine;
                    aColumn = 1;
                }
                else
                {
                    ++aColumn;
                }
            }
        }

        std::string escapeString(const std::string& aText)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            writer.String(aText.data(), static_cast<rapidjson::SizeType>(aText.size()));
            return std::string(buffer.GetString(), buffer.GetSize());
        }

        bool parsesToFloat(const char* aText, float aExpected)
        {
            rapidjson::Document document;
            document.Parse<rapidjson::kParseFullPrecisionFlag>(aText);
            return !document.HasParseError() && document.IsNumber() && static_cast<float>(document.GetDouble()) == aExpected;
        }

        std::string formatDouble(double aValue)
        {
            if (!std::isfinite(aValue))
            {
                return "null";
            }
            if (std::abs(aValue) < 1e15 && std::floor(aValue) == aValue)
            {
                return std::to_string(static_cast<int64_t>(aValue));
            }
            char buffer[64] = {};
            const float asFloat = static_cast<float>(aValue);
            if (static_cast<double>(asFloat) == aValue)
            {
                for (int decimals = 1; decimals <= 12; ++decimals)
                {
                    char* end = rapidjson::internal::dtoa(aValue, buffer, decimals);
                    *end = '\0';
                    if (parsesToFloat(buffer, asFloat))
                    {
                        return buffer;
                    }
                }
            }
            char* end = rapidjson::internal::dtoa(aValue, buffer);
            *end = '\0';
            return buffer;
        }

        bool isScalar(const Value& aValue)
        {
            return !aValue.isArray() && !aValue.isObject();
        }

        bool fitsInline(const ValueArray& aArray)
        {
            if (aArray.size() > kMaxInlineArrayLength)
            {
                return false;
            }
            for (const auto& element : aArray)
            {
                if (!isScalar(element))
                {
                    return false;
                }
            }
            return true;
        }

        constexpr size_t kMaxInlineObjectMembers = 6;
        constexpr size_t kMaxInlineObjectLength = 96;

        bool fitsInline(const ValueObject& aObject)
        {
            if (aObject.size() > kMaxInlineObjectMembers)
            {
                return false;
            }
            size_t length = 2;
            for (const auto& member : aObject)
            {
                if (member.value.isObject())
                {
                    return false;
                }
                if (const ValueArray* array = member.value.getArray())
                {
                    if (!fitsInline(*array))
                    {
                        return false;
                    }
                }
                length += member.key.size() + 6;
                length += member.value.isString() ? member.value.asString().size() : 8;
            }
            return length <= kMaxInlineObjectLength;
        }

        void writeIndent(std::string& aOut, int aDepth)
        {
            aOut.append(static_cast<size_t>(aDepth), '\t');
        }

        void writeValue(std::string& aOut, const Value& aValue, int aDepth, bool aPretty)
        {
            switch (aValue.getType())
            {
            case Value::Type::Null:
                aOut += "null";
                return;
            case Value::Type::Bool:
                aOut += aValue.asBool() ? "true" : "false";
                return;
            case Value::Type::Int:
                aOut += std::to_string(aValue.asInt());
                return;
            case Value::Type::Double:
                aOut += formatDouble(aValue.asDouble());
                return;
            case Value::Type::String:
                aOut += escapeString(aValue.asString());
                return;
            case Value::Type::Array:
            {
                const ValueArray& array = aValue.asArray();
                if (array.empty())
                {
                    aOut += "[]";
                    return;
                }
                const bool inlineArray = !aPretty || fitsInline(array);
                aOut += '[';
                for (size_t index = 0; index < array.size(); ++index)
                {
                    if (index > 0)
                    {
                        aOut += inlineArray ? (aPretty ? ", " : ",") : ",";
                    }
                    if (!inlineArray)
                    {
                        aOut += '\n';
                        writeIndent(aOut, aDepth + 1);
                    }
                    writeValue(aOut, array[index], aDepth + 1, aPretty);
                }
                if (!inlineArray)
                {
                    aOut += '\n';
                    writeIndent(aOut, aDepth);
                }
                aOut += ']';
                return;
            }
            case Value::Type::Object:
            {
                const ValueObject& object = aValue.asObject();
                if (object.empty())
                {
                    aOut += "{}";
                    return;
                }
                const bool multiline = aPretty && (aDepth == 0 || !fitsInline(object));
                aOut += '{';
                bool first = true;
                for (const auto& member : object)
                {
                    if (!first)
                    {
                        aOut += (aPretty && !multiline) ? ", " : ",";
                    }
                    first = false;
                    if (multiline)
                    {
                        aOut += '\n';
                        writeIndent(aOut, aDepth + 1);
                    }
                    aOut += escapeString(member.key);
                    aOut += aPretty ? ": " : ":";
                    writeValue(aOut, member.value, aDepth + 1, aPretty);
                }
                if (multiline)
                {
                    aOut += '\n';
                    writeIndent(aOut, aDepth);
                }
                aOut += '}';
                return;
            }
            }
        }
    }

    Result<Value> parseJson(std::string_view aText)
    {
        rapidjson::Document document;
        document.Parse<kParseFlags>(aText.data(), aText.size());
        if (document.HasParseError())
        {
            size_t line = 0;
            size_t column = 0;
            describeOffset(aText, document.GetErrorOffset(), line, column);
            std::string message = "JSON parse error at line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + rapidjson::GetParseError_En(document.GetParseError());
            return Result<Value>::failure(std::move(message));
        }
        return Result<Value>::success(convertFromRapid(document));
    }

    std::string writeJson(const Value& aValue)
    {
        std::string out;
        writeValue(out, aValue, 0, true);
        out += '\n';
        return out;
    }

    std::string writeJsonCompact(const Value& aValue)
    {
        std::string out;
        writeValue(out, aValue, 0, false);
        return out;
    }
}
