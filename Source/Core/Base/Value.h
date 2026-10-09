#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace hg
{
    class Value;
    struct ValueMember;

    using ValueArray = std::vector<Value>;

    class ValueObject
    {
    public:
        using Members = std::vector<ValueMember>;

        ValueObject();
        ValueObject(std::initializer_list<ValueMember> aMembers);
        ValueObject(const ValueObject& aOther);
        ValueObject(ValueObject&& aOther) noexcept;
        ValueObject& operator=(const ValueObject& aOther);
        ValueObject& operator=(ValueObject&& aOther) noexcept;
        ~ValueObject();

        const Value* find(std::string_view aKey) const;
        Value* find(std::string_view aKey);
        bool contains(std::string_view aKey) const;
        const Value& get(std::string_view aKey) const;
        void set(std::string_view aKey, Value aValue);
        bool erase(std::string_view aKey);
        Value& operator[](std::string_view aKey);

        size_t size() const;
        bool empty() const;
        void clear();

        Members::const_iterator begin() const;
        Members::const_iterator end() const;
        Members::iterator begin();
        Members::iterator end();

        bool operator==(const ValueObject& aOther) const;

    private:
        Members mMembers;
    };

    class Value
    {
    public:
        enum class Type
        {
            Null,
            Bool,
            Int,
            Double,
            String,
            Array,
            Object
        };

        Value() = default;
        Value(std::nullptr_t);
        Value(bool aValue);

        template <std::integral T>
            requires(!std::same_as<T, bool>)
        Value(T aValue)
            : mData(static_cast<int64_t>(aValue))
        {
        }

        template <std::floating_point T>
        Value(T aValue)
            : mData(static_cast<double>(aValue))
        {
        }

        Value(const char* aValue);
        Value(std::string_view aValue);
        Value(std::string aValue);
        Value(ValueArray aValue);
        Value(ValueObject aValue);

        Type getType() const;
        bool isNull() const;
        bool isBool() const;
        bool isInt() const;
        bool isDouble() const;
        bool isNumber() const;
        bool isString() const;
        bool isArray() const;
        bool isObject() const;

        bool asBool(bool aFallback = false) const;
        int64_t asInt(int64_t aFallback = 0) const;
        double asDouble(double aFallback = 0.0) const;
        float asFloat(float aFallback = 0.0f) const;
        const std::string& asString() const;
        std::string asString(std::string_view aFallback) const;

        const ValueArray& asArray() const;
        const ValueObject& asObject() const;

        ValueArray* getArray();
        const ValueArray* getArray() const;
        ValueObject* getObject();
        const ValueObject* getObject() const;

        bool operator==(const Value& aOther) const;

        static const Value& null();

    private:
        std::variant<std::monostate, bool, int64_t, double, std::string, ValueArray, ValueObject> mData;
    };

    struct ValueMember
    {
        std::string key;
        Value value;

        bool operator==(const ValueMember& aOther) const = default;
    };

    inline ValueObject::Members::const_iterator ValueObject::begin() const
    {
        return mMembers.begin();
    }

    inline ValueObject::Members::const_iterator ValueObject::end() const
    {
        return mMembers.end();
    }

    inline ValueObject::Members::iterator ValueObject::begin()
    {
        return mMembers.begin();
    }

    inline ValueObject::Members::iterator ValueObject::end()
    {
        return mMembers.end();
    }

    std::string_view valueTypeName(Value::Type aType);
}
