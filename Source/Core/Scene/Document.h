#pragma once

#include "Core/Animation/Timeline.h"
#include "Core/Base/Ids.h"
#include "Core/Base/Value.h"
#include "Core/Scene/Schema.h"

#include <functional>
#include <string>
#include <vector>

namespace hg
{
    struct ComponentDesc
    {
        std::string type;
        ValueObject props;

        bool operator==(const ComponentDesc& aOther) const = default;
    };

    struct ObjectDesc
    {
        ObjectId uid = kInvalidObjectId;
        std::string name;
        std::string type;
        std::string prefab;
        ValueObject props;
        std::vector<ComponentDesc> components;
        std::vector<ObjectDesc> children;

        const ComponentDesc* findComponent(std::string_view aType) const;
        ComponentDesc* findComponent(std::string_view aType);

        bool operator==(const ObjectDesc& aOther) const = default;
    };

    struct SceneDocument
    {
        static constexpr int kCurrentVersion = 2;

        int version = kCurrentVersion;
        std::string id;
        SceneKind kind = SceneKind::Location;
        ObjectId nextUid = 1;
        ValueObject settings;
        std::vector<ObjectDesc> objects;
        std::vector<TimelineDesc> timelines;
        ValueObject extra;

        ObjectDesc* findObject(ObjectId aUid);
        const ObjectDesc* findObject(ObjectId aUid) const;
        ObjectDesc* findParent(ObjectId aUid);
        const ObjectDesc* findParent(ObjectId aUid) const;
        std::vector<ObjectDesc>* findSiblings(ObjectId aUid);
        std::vector<ObjectDesc>* childrenOf(ObjectId aParentUid);
        TimelineDesc* findTimeline(std::string_view aId);
        const TimelineDesc* findTimeline(std::string_view aId) const;

        bool isAncestor(ObjectId aAncestor, ObjectId aDescendant) const;
        ObjectId allocateUid();
        ObjectId computeMaxUid() const;
        void normalizeUids();

        bool operator==(const SceneDocument& aOther) const = default;
    };

    void forEachObject(std::vector<ObjectDesc>& aObjects, const std::function<void(ObjectDesc&, ObjectDesc*)>& aVisitor, ObjectDesc* aParent = nullptr);
    void forEachObject(const std::vector<ObjectDesc>& aObjects, const std::function<void(const ObjectDesc&, const ObjectDesc*)>& aVisitor, const ObjectDesc* aParent = nullptr);
    void reassignUids(ObjectDesc& aObject, SceneDocument& aDocument);
    std::string makeUniqueName(const std::vector<ObjectDesc>& aSiblings, std::string_view aBaseName);
}
