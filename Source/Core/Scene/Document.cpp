#include "Core/Scene/Document.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace hg
{
    namespace
    {
        template <typename TObject>
        TObject* findInTree(std::vector<ObjectDesc>& aObjects, ObjectId aUid)
        {
            for (auto& object : aObjects)
            {
                if (object.uid == aUid)
                {
                    return &object;
                }
                if (TObject* found = findInTree<TObject>(object.children, aUid))
                {
                    return found;
                }
            }
            return nullptr;
        }

        const ObjectDesc* findInTreeConst(const std::vector<ObjectDesc>& aObjects, ObjectId aUid)
        {
            for (const auto& object : aObjects)
            {
                if (object.uid == aUid)
                {
                    return &object;
                }
                if (const ObjectDesc* found = findInTreeConst(object.children, aUid))
                {
                    return found;
                }
            }
            return nullptr;
        }

        ObjectDesc* findParentInTree(std::vector<ObjectDesc>& aObjects, ObjectId aUid, ObjectDesc* aParent, bool& aFound)
        {
            for (auto& object : aObjects)
            {
                if (object.uid == aUid)
                {
                    aFound = true;
                    return aParent;
                }
                ObjectDesc* result = findParentInTree(object.children, aUid, &object, aFound);
                if (aFound)
                {
                    return result;
                }
            }
            return nullptr;
        }

        ObjectId maxUidIn(const std::vector<ObjectDesc>& aObjects)
        {
            ObjectId maxUid = kInvalidObjectId;
            for (const auto& object : aObjects)
            {
                maxUid = std::max(maxUid, object.uid);
                maxUid = std::max(maxUid, maxUidIn(object.children));
            }
            return maxUid;
        }

        void fixDuplicateUids(std::vector<ObjectDesc>& aObjects, std::unordered_set<ObjectId>& aSeen, SceneDocument& aDocument)
        {
            for (auto& object : aObjects)
            {
                if (object.uid == kInvalidObjectId || !aSeen.insert(object.uid).second)
                {
                    object.uid = aDocument.allocateUid();
                    aSeen.insert(object.uid);
                }
                fixDuplicateUids(object.children, aSeen, aDocument);
            }
        }
    }

    const ComponentDesc* ObjectDesc::findComponent(std::string_view aType) const
    {
        auto it = std::find_if(components.begin(), components.end(), [aType](const ComponentDesc& aComponent)
        {
            return aComponent.type == aType;
        });
        return it != components.end() ? &*it : nullptr;
    }

    ComponentDesc* ObjectDesc::findComponent(std::string_view aType)
    {
        auto it = std::find_if(components.begin(), components.end(), [aType](const ComponentDesc& aComponent)
        {
            return aComponent.type == aType;
        });
        return it != components.end() ? &*it : nullptr;
    }

    ObjectDesc* SceneDocument::findObject(ObjectId aUid)
    {
        return aUid == kInvalidObjectId ? nullptr : findInTree<ObjectDesc>(objects, aUid);
    }

    const ObjectDesc* SceneDocument::findObject(ObjectId aUid) const
    {
        return aUid == kInvalidObjectId ? nullptr : findInTreeConst(objects, aUid);
    }

    ObjectDesc* SceneDocument::findParent(ObjectId aUid)
    {
        bool found = false;
        return findParentInTree(objects, aUid, nullptr, found);
    }

    const ObjectDesc* SceneDocument::findParent(ObjectId aUid) const
    {
        return const_cast<SceneDocument*>(this)->findParent(aUid);
    }

    std::vector<ObjectDesc>* SceneDocument::findSiblings(ObjectId aUid)
    {
        if (!findObject(aUid))
        {
            return nullptr;
        }
        ObjectDesc* parent = findParent(aUid);
        return parent ? &parent->children : &objects;
    }

    std::vector<ObjectDesc>* SceneDocument::childrenOf(ObjectId aParentUid)
    {
        if (aParentUid == kInvalidObjectId)
        {
            return &objects;
        }
        ObjectDesc* parent = findObject(aParentUid);
        return parent ? &parent->children : nullptr;
    }

    TimelineDesc* SceneDocument::findTimeline(std::string_view aId)
    {
        auto it = std::find_if(timelines.begin(), timelines.end(), [aId](const TimelineDesc& aTimeline)
        {
            return aTimeline.id == aId;
        });
        return it != timelines.end() ? &*it : nullptr;
    }

    const TimelineDesc* SceneDocument::findTimeline(std::string_view aId) const
    {
        return const_cast<SceneDocument*>(this)->findTimeline(aId);
    }

    bool SceneDocument::isAncestor(ObjectId aAncestor, ObjectId aDescendant) const
    {
        const ObjectDesc* ancestor = findObject(aAncestor);
        return ancestor && findInTreeConst(ancestor->children, aDescendant) != nullptr;
    }

    ObjectId SceneDocument::allocateUid()
    {
        nextUid = std::max(nextUid, computeMaxUid() + 1);
        return nextUid++;
    }

    ObjectId SceneDocument::computeMaxUid() const
    {
        return maxUidIn(objects);
    }

    void SceneDocument::normalizeUids()
    {
        nextUid = std::max<ObjectId>(nextUid, computeMaxUid() + 1);
        std::unordered_set<ObjectId> seen;
        fixDuplicateUids(objects, seen, *this);
    }

    void forEachObject(std::vector<ObjectDesc>& aObjects, const std::function<void(ObjectDesc&, ObjectDesc*)>& aVisitor, ObjectDesc* aParent)
    {
        for (auto& object : aObjects)
        {
            aVisitor(object, aParent);
            forEachObject(object.children, aVisitor, &object);
        }
    }

    void forEachObject(const std::vector<ObjectDesc>& aObjects, const std::function<void(const ObjectDesc&, const ObjectDesc*)>& aVisitor, const ObjectDesc* aParent)
    {
        for (const auto& object : aObjects)
        {
            aVisitor(object, aParent);
            forEachObject(object.children, aVisitor, &object);
        }
    }

    void reassignUids(ObjectDesc& aObject, SceneDocument& aDocument)
    {
        aObject.uid = aDocument.allocateUid();
        for (auto& child : aObject.children)
        {
            reassignUids(child, aDocument);
        }
    }

    std::string makeUniqueName(const std::vector<ObjectDesc>& aSiblings, std::string_view aBaseName)
    {
        auto taken = [&aSiblings](const std::string& aName)
        {
            return std::any_of(aSiblings.begin(), aSiblings.end(), [&aName](const ObjectDesc& aObject)
            {
                return aObject.name == aName;
            });
        };

        std::string base(aBaseName);
        while (!base.empty() && std::isdigit(static_cast<unsigned char>(base.back())))
        {
            base.pop_back();
        }
        if (!base.empty() && base.back() == '_')
        {
            base.pop_back();
        }
        if (base.empty())
        {
            base = "Object";
        }

        if (!taken(std::string(aBaseName)) && !aBaseName.empty())
        {
            return std::string(aBaseName);
        }
        for (int index = 1; index < 100000; ++index)
        {
            std::string candidate = base + "_" + std::to_string(index);
            if (!taken(candidate))
            {
                return candidate;
            }
        }
        return base;
    }
}
