#pragma once

#include <cstdint>

namespace hg
{
    using ObjectId = uint64_t;

    inline constexpr ObjectId kInvalidObjectId = 0;
    inline constexpr ObjectId kPrefabChildIdBits = 24;
    inline constexpr ObjectId kPrefabChildIdMask = (ObjectId(1) << kPrefabChildIdBits) - 1;

    inline constexpr ObjectId combinePrefabIds(ObjectId aInstanceId, ObjectId aPrefabChildId)
    {
        return (aInstanceId << kPrefabChildIdBits) | (aPrefabChildId & kPrefabChildIdMask);
    }
}
