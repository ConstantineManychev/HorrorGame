#pragma once

#include <optional>
#include <string>
#include <utility>

namespace hg
{
    template <typename T>
    class Result
    {
    public:
        static Result success(T aValue)
        {
            Result result;
            result.mValue = std::move(aValue);
            return result;
        }

        static Result failure(std::string aError)
        {
            Result result;
            result.mError = std::move(aError);
            return result;
        }

        bool ok() const
        {
            return mValue.has_value();
        }

        explicit operator bool() const
        {
            return ok();
        }

        const T& value() const
        {
            return *mValue;
        }

        T& value()
        {
            return *mValue;
        }

        T take()
        {
            return std::move(*mValue);
        }

        const std::string& error() const
        {
            return mError;
        }

    private:
        std::optional<T> mValue;
        std::string mError;
    };
}
