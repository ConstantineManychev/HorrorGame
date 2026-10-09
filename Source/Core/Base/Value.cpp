#include "Core/Base/Value.h"

#include <algorithm>
#include <cmath>

namespace hg
{
    ValueObject::ValueObject() = default;
    ValueObject::ValueObject(const ValueObject& aOther) = default;
    ValueObject::ValueObject(ValueObject&& aOther) noexcept = default;
    ValueObject& ValueObject::operator=(const ValueObject& aOther) = default;
    ValueObject& ValueObject::operator=(ValueObject&& aOther) noexcept = default;
    ValueObject::~ValueObject() = default;

    ValueObject::ValueObject(std::initializer_list<ValueMember> aMembers)
    {
        for (const auto& member : aMembers)
        {
            set(member.key, member.value);
        }
    }

    const Value* ValueObject::find(std::string_view aKey) const
    {
        for (const auto& member : mMembers)
        {
            if (member.key == aKey)
            {
                return &member.value;
            }
        }
        return nullptr;
    }

    Value* ValueObject::find(std::string_view aKey)
    {
        for (auto& member : mMembers)
        {
            if (member.key == aKey)
            {
                return &member.value;
            }
        }
        return nullptr;
    }

    bool ValueObject::contains(std::string_view aKey) const
    {
        return find(aKey) != nullptr;
    }

    const Value& ValueObject::get(std::string_view aKey) const
    {
        const Value* found = find(aKey);
        return found ? *found : Value::null();
    }

    void ValueObject::set(std::string_view aKey, Value aValue)
    {
        if (Value* existing = find(aKey))
        {
            *existing = std::move(aValue);
            return;
        }
        mMembers.push_back(ValueMember{std::string(aKey), std::move(aValue)});
    }

    bool ValueObject::erase(std::string_view aKey)
    {
        auto it = std::find_if(mMembers.begin(), mMembers.end(), [aKey](const ValueMember& aMember)
        {
            return aMember.key == aKey;
        });
        if (it == mMembers.end())
        {
            return false;
        }
        mMembers.erase(it);
        return true;
    }

    Value& ValueObject::operator[](std::string_view aKey)
    {
        if (Value* existing = find(aKey))
        {
            return *existing;
        }
        mMembers.push_back(ValueMember{std::string(aKey), Value()});
        return mMembers.back().value;
    }

    size_t ValueObject::size() const
    {
        return mMembers.size();
    }

    bool ValueObject::empty() const
    {
        return mMembers.empty();
    }

    void ValueObject::clear()
    {
        mMembers.clear();
    }

    bool ValueObject::operator==(const ValueObject& aOther) const
    {
        if (mMembers.size() != aOther.mMembers.size())
        {
            return false;
        }
        for (const auto& member : mMembers)
        {
            const Value* other = aOther.find(member.key);
            if (!other || !(*other == member.value))
            {
                return false;
            }
        }
        return true;
    }

    Value::Value(std::nullptr_t)
        : mData(std::monostate{})
    {
    }

    Value::Value(bool aValue)
        : mData(aValue)
    {
    }

    Value::Value(const char* aValue)
        : mData(std::string(aValue ? aValue : ""))
    {
    }

    Value::Value(std::string_view aValue)
        : mData(std::string(aValue))
    {
    }

    Value::Value(std::string aValue)
        : mData(std::move(aValue))
    {
    }

    Value::Value(ValueArray aValue)
        : mData(std::move(aValue))
    {
    }

    Value::Value(ValueObject aValue)
        : mData(std::move(aValue))
    {
    }

    Value::Type Value::getType() const
    {
        return static_cast<Type>(mData.index());
    }

    bool Value::isNull() const
    {
        return getType() == Type::Null;
    }

    bool Value::isBool() const
    {
        return getType() == Type::Bool;
    }

    bool Value::isInt() const
    {
        return getType() == Type::Int;
    }

    bool Value::isDouble() const
    {
        return getType() == Type::Double;
    }

    bool Value::isNumber() const
    {
        return isInt() || isDouble();
    }

    bool Value::isString() const
    {
        return getType() == Type::String;
    }

    bool Value::isArray() const
    {
        return getType() == Type::Array;
    }

    bool Value::isObject() const
    {
        return getType() == Type::Object;
    }

    bool Value::asBool(bool aFallback) const
    {
        if (const bool* value = std::get_if<bool>(&mData))
        {
            return *value;
        }
        if (const int64_t* value = std::get_if<int64_t>(&mData))
        {
            return *value != 0;
        }
        return aFallback;
    }

    int64_t Value::asInt(int64_t aFallback) const
    {
        if (const int64_t* value = std::get_if<int64_t>(&mData))
        {
            return *value;
        }
        if (const double* value = std::get_if<double>(&mData))
        {
            return static_cast<int64_t>(std::llround(*value));
        }
        if (const bool* value = std::get_if<bool>(&mData))
        {
            return *value ? 1 : 0;
        }
        return aFallback;
    }

    double Value::asDouble(double aFallback) const
    {
        if (const double* value = std::get_if<double>(&mData))
        {
            return *value;
        }
        if (const int64_t* value = std::get_if<int64_t>(&mData))
        {
            return static_cast<double>(*value);
        }
        return aFallback;
    }

    float Value::asFloat(float aFallback) const
    {
        return static_cast<float>(asDouble(aFallback));
    }

    const std::string& Value::asString() const
    {
        static const std::string empty;
        const std::string* value = std::get_if<std::string>(&mData);
        return value ? *value : empty;
    }

    std::string Value::asString(std::string_view aFallback) const
    {
        const std::string* value = std::get_if<std::string>(&mData);
        return value ? *value : std::string(aFallback);
    }

    const ValueArray& Value::asArray() const
    {
        static const ValueArray empty;
        const ValueArray* value = std::get_if<ValueArray>(&mData);
        return value ? *value : empty;
    }

    const ValueObject& Value::asObject() const
    {
        static const ValueObject empty;
        const ValueObject* value = std::get_if<ValueObject>(&mData);
        return value ? *value : empty;
    }

    ValueArray* Value::getArray()
    {
        return std::get_if<ValueArray>(&mData);
    }

    const ValueArray* Value::getArray() const
    {
        return std::get_if<ValueArray>(&mData);
    }

    ValueObject* Value::getObject()
    {
        return std::get_if<ValueObject>(&mData);
    }

    const ValueObject* Value::getObject() const
    {
        return std::get_if<ValueObject>(&mData);
    }

    bool Value::operator==(const Value& aOther) const
    {
        if (isNumber() && aOther.isNumber())
        {
            if (isInt() && aOther.isInt())
            {
                return asInt() == aOther.asInt();
            }
            return asDouble() == aOther.asDouble();
        }
        return mData == aOther.mData;
    }

    const Value& Value::null()
    {
        static const Value nullValue;
        return nullValue;
    }

    std::string_view valueTypeName(Value::Type aType)
    {
        switch (aType)
        {
        case Value::Type::Null:
            return "null";
        case Value::Type::Bool:
            return "bool";
        case Value::Type::Int:
            return "int";
        case Value::Type::Double:
            return "double";
        case Value::Type::String:
            return "string";
        case Value::Type::Array:
            return "array";
        case Value::Type::Object:
            return "object";
        }
        return "unknown";
    }
}
