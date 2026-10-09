#pragma once

#include "Core/Base/Result.h"
#include "Core/Base/Value.h"

#include <string>
#include <string_view>

namespace hg
{
    Result<Value> parseJson(std::string_view aText);
    std::string writeJson(const Value& aValue);
    std::string writeJsonCompact(const Value& aValue);
}
