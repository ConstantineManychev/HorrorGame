#include "Core/Edit/SceneCommands.h"

#include <algorithm>

namespace hg
{
    namespace
    {
        std::optional<Value> readOptional(const ValueObject& aObject, std::string_view aKey)
        {
            const Value* value = aObject.find(aKey);
            return value ? std::optional<Value>(*value) : std::nullopt;
        }

        void writeOptional(ValueObject& aObject, std::string_view aKey, const std::optional<Value>& aValue)
        {
            if (aValue)
            {
                aObject.set(aKey, *aValue);
            }
            else
            {
                aObject.erase(aKey);
            }
        }

        ObjectId parentUidOf(SceneDocument& aDocument, ObjectId aUid)
        {
            ObjectDesc* parent = aDocument.findParent(aUid);
            return parent ? parent->uid : kInvalidObjectId;
        }

        size_t indexIn(const std::vector<ObjectDesc>& aSiblings, ObjectId aUid)
        {
            auto it = std::find_if(aSiblings.begin(), aSiblings.end(), [aUid](const ObjectDesc& aObject)
            {
                return aObject.uid == aUid;
            });
            return static_cast<size_t>(std::distance(aSiblings.begin(), it));
        }
    }

    SetPropertyCommand::SetPropertyCommand(ObjectId aUid, std::string aProperty, std::optional<Value> aValue)
        : mUid(aUid)
        , mProperty(std::move(aProperty))
        , mNewValue(std::move(aValue))
    {
    }

    std::string SetPropertyCommand::label() const
    {
        return "Change " + mProperty;
    }

    bool SetPropertyCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        ObjectDesc* object = aDocument.findObject(mUid);
        if (!object)
        {
            return false;
        }
        mOldValue = readOptional(object->props, mProperty);
        assign(aDocument, mNewValue, aChanges);
        return true;
    }

    void SetPropertyCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        assign(aDocument, mOldValue, aChanges);
    }

    bool SetPropertyCommand::canMergeWith(const Command& aNext) const
    {
        const auto* next = dynamic_cast<const SetPropertyCommand*>(&aNext);
        return next && next->mUid == mUid && next->mProperty == mProperty;
    }

    void SetPropertyCommand::mergeWith(Command& aNext)
    {
        mNewValue = static_cast<SetPropertyCommand&>(aNext).mNewValue;
    }

    void SetPropertyCommand::assign(SceneDocument& aDocument, const std::optional<Value>& aValue, ChangeSet& aChanges)
    {
        if (ObjectDesc* object = aDocument.findObject(mUid))
        {
            writeOptional(object->props, mProperty, aValue);
            aChanges.addProperty(mUid, mProperty);
        }
    }

    RenameObjectCommand::RenameObjectCommand(ObjectId aUid, std::string aName)
        : mUid(aUid)
        , mNewName(std::move(aName))
    {
    }

    std::string RenameObjectCommand::label() const
    {
        return "Rename to " + mNewName;
    }

    bool RenameObjectCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        ObjectDesc* object = aDocument.findObject(mUid);
        if (!object || object->name == mNewName)
        {
            return false;
        }
        mOldName = object->name;
        object->name = mNewName;
        aChanges.addObject(mUid);
        return true;
    }

    void RenameObjectCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (ObjectDesc* object = aDocument.findObject(mUid))
        {
            object->name = mOldName;
            aChanges.addObject(mUid);
        }
    }

    CreateObjectCommand::CreateObjectCommand(ObjectId aParentUid, size_t aIndex, ObjectDesc aObject)
        : mParentUid(aParentUid)
        , mIndex(aIndex)
        , mObject(std::move(aObject))
    {
    }

    std::string CreateObjectCommand::label() const
    {
        return "Create " + mObject.name;
    }

    bool CreateObjectCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        std::vector<ObjectDesc>* siblings = aDocument.childrenOf(mParentUid);
        if (!siblings || mObject.uid == kInvalidObjectId || aDocument.findObject(mObject.uid))
        {
            return false;
        }
        const size_t index = std::min(mIndex, siblings->size());
        siblings->insert(siblings->begin() + static_cast<std::ptrdiff_t>(index), mObject);
        aDocument.nextUid = std::max(aDocument.nextUid, aDocument.computeMaxUid() + 1);
        aChanges.structure = true;
        return true;
    }

    void CreateObjectCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (std::vector<ObjectDesc>* siblings = aDocument.findSiblings(mObject.uid))
        {
            siblings->erase(siblings->begin() + static_cast<std::ptrdiff_t>(indexIn(*siblings, mObject.uid)));
            aChanges.structure = true;
        }
    }

    ObjectId CreateObjectCommand::createdUid() const
    {
        return mObject.uid;
    }

    DeleteObjectCommand::DeleteObjectCommand(ObjectId aUid)
        : mUid(aUid)
    {
    }

    std::string DeleteObjectCommand::label() const
    {
        return "Delete " + mObject.name;
    }

    bool DeleteObjectCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        std::vector<ObjectDesc>* siblings = aDocument.findSiblings(mUid);
        if (!siblings)
        {
            return false;
        }
        mParentUid = parentUidOf(aDocument, mUid);
        mIndex = indexIn(*siblings, mUid);
        mObject = (*siblings)[mIndex];
        siblings->erase(siblings->begin() + static_cast<std::ptrdiff_t>(mIndex));
        aChanges.structure = true;
        return true;
    }

    void DeleteObjectCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (std::vector<ObjectDesc>* siblings = aDocument.childrenOf(mParentUid))
        {
            const size_t index = std::min(mIndex, siblings->size());
            siblings->insert(siblings->begin() + static_cast<std::ptrdiff_t>(index), mObject);
            aChanges.structure = true;
        }
    }

    MoveObjectCommand::MoveObjectCommand(ObjectId aUid, ObjectId aNewParentUid, size_t aNewIndex)
        : mUid(aUid)
        , mNewParentUid(aNewParentUid)
        , mNewIndex(aNewIndex)
    {
    }

    std::string MoveObjectCommand::label() const
    {
        return "Move in hierarchy";
    }

    bool MoveObjectCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (mUid == mNewParentUid || aDocument.isAncestor(mUid, mNewParentUid))
        {
            return false;
        }
        std::vector<ObjectDesc>* siblings = aDocument.findSiblings(mUid);
        if (!siblings || !aDocument.childrenOf(mNewParentUid))
        {
            return false;
        }
        mOldParentUid = parentUidOf(aDocument, mUid);
        mOldIndex = indexIn(*siblings, mUid);
        size_t finalIndex = mNewIndex;
        if (mOldParentUid == mNewParentUid && mOldIndex < mNewIndex)
        {
            --finalIndex;
        }
        if (mOldParentUid == mNewParentUid && finalIndex == mOldIndex)
        {
            return false;
        }
        if (!move(aDocument, mUid, mNewParentUid, finalIndex))
        {
            return false;
        }
        aChanges.structure = true;
        return true;
    }

    void MoveObjectCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (move(aDocument, mUid, mOldParentUid, mOldIndex))
        {
            aChanges.structure = true;
        }
    }

    bool MoveObjectCommand::move(SceneDocument& aDocument, ObjectId aUid, ObjectId aParentUid, size_t aFinalIndex)
    {
        std::vector<ObjectDesc>* siblings = aDocument.findSiblings(aUid);
        if (!siblings || !aDocument.childrenOf(aParentUid))
        {
            return false;
        }
        const size_t oldIndex = indexIn(*siblings, aUid);
        ObjectDesc object = std::move((*siblings)[oldIndex]);
        siblings->erase(siblings->begin() + static_cast<std::ptrdiff_t>(oldIndex));

        std::vector<ObjectDesc>* target = aDocument.childrenOf(aParentUid);
        const size_t index = std::min(aFinalIndex, target->size());
        target->insert(target->begin() + static_cast<std::ptrdiff_t>(index), std::move(object));
        return true;
    }

    AddComponentCommand::AddComponentCommand(ObjectId aUid, ComponentDesc aComponent)
        : mUid(aUid)
        , mComponent(std::move(aComponent))
    {
    }

    std::string AddComponentCommand::label() const
    {
        return "Add " + mComponent.type;
    }

    bool AddComponentCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        ObjectDesc* object = aDocument.findObject(mUid);
        if (!object || object->findComponent(mComponent.type))
        {
            return false;
        }
        object->components.push_back(mComponent);
        aChanges.addObject(mUid);
        return true;
    }

    void AddComponentCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (ObjectDesc* object = aDocument.findObject(mUid))
        {
            auto it = std::find_if(object->components.begin(), object->components.end(), [this](const ComponentDesc& aComponent)
            {
                return aComponent.type == mComponent.type;
            });
            if (it != object->components.end())
            {
                object->components.erase(it);
                aChanges.addObject(mUid);
            }
        }
    }

    RemoveComponentCommand::RemoveComponentCommand(ObjectId aUid, size_t aIndex)
        : mUid(aUid)
        , mIndex(aIndex)
    {
    }

    std::string RemoveComponentCommand::label() const
    {
        return "Remove " + mComponent.type;
    }

    bool RemoveComponentCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        ObjectDesc* object = aDocument.findObject(mUid);
        if (!object || mIndex >= object->components.size())
        {
            return false;
        }
        mComponent = object->components[mIndex];
        object->components.erase(object->components.begin() + static_cast<std::ptrdiff_t>(mIndex));
        aChanges.addObject(mUid);
        return true;
    }

    void RemoveComponentCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (ObjectDesc* object = aDocument.findObject(mUid))
        {
            const size_t index = std::min(mIndex, object->components.size());
            object->components.insert(object->components.begin() + static_cast<std::ptrdiff_t>(index), mComponent);
            aChanges.addObject(mUid);
        }
    }

    SetComponentPropertyCommand::SetComponentPropertyCommand(ObjectId aUid, size_t aComponentIndex, std::string aProperty, std::optional<Value> aValue)
        : mUid(aUid)
        , mComponentIndex(aComponentIndex)
        , mProperty(std::move(aProperty))
        , mNewValue(std::move(aValue))
    {
    }

    std::string SetComponentPropertyCommand::label() const
    {
        return "Change " + mProperty;
    }

    bool SetComponentPropertyCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        ObjectDesc* object = aDocument.findObject(mUid);
        if (!object || mComponentIndex >= object->components.size())
        {
            return false;
        }
        mOldValue = readOptional(object->components[mComponentIndex].props, mProperty);
        return assign(aDocument, mNewValue, aChanges);
    }

    void SetComponentPropertyCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        assign(aDocument, mOldValue, aChanges);
    }

    bool SetComponentPropertyCommand::canMergeWith(const Command& aNext) const
    {
        const auto* next = dynamic_cast<const SetComponentPropertyCommand*>(&aNext);
        return next && next->mUid == mUid && next->mComponentIndex == mComponentIndex && next->mProperty == mProperty;
    }

    void SetComponentPropertyCommand::mergeWith(Command& aNext)
    {
        mNewValue = static_cast<SetComponentPropertyCommand&>(aNext).mNewValue;
    }

    bool SetComponentPropertyCommand::assign(SceneDocument& aDocument, const std::optional<Value>& aValue, ChangeSet& aChanges)
    {
        ObjectDesc* object = aDocument.findObject(mUid);
        if (!object || mComponentIndex >= object->components.size())
        {
            return false;
        }
        writeOptional(object->components[mComponentIndex].props, mProperty, aValue);
        aChanges.addObject(mUid);
        return true;
    }

    SetSettingCommand::SetSettingCommand(std::string aKey, std::optional<Value> aValue)
        : mKey(std::move(aKey))
        , mNewValue(std::move(aValue))
    {
    }

    std::string SetSettingCommand::label() const
    {
        return "Change setting " + mKey;
    }

    bool SetSettingCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        mOldValue = readOptional(aDocument.settings, mKey);
        writeOptional(aDocument.settings, mKey, mNewValue);
        aChanges.settings = true;
        return true;
    }

    void SetSettingCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        writeOptional(aDocument.settings, mKey, mOldValue);
        aChanges.settings = true;
    }

    bool SetSettingCommand::canMergeWith(const Command& aNext) const
    {
        const auto* next = dynamic_cast<const SetSettingCommand*>(&aNext);
        return next && next->mKey == mKey;
    }

    void SetSettingCommand::mergeWith(Command& aNext)
    {
        mNewValue = static_cast<SetSettingCommand&>(aNext).mNewValue;
    }

    SetTimelinesCommand::SetTimelinesCommand(std::vector<TimelineDesc> aTimelines, std::string aLabel, std::string aMergeKey)
        : mNewTimelines(std::move(aTimelines))
        , mLabel(std::move(aLabel))
        , mMergeKey(std::move(aMergeKey))
    {
    }

    std::string SetTimelinesCommand::label() const
    {
        return mLabel;
    }

    bool SetTimelinesCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (aDocument.timelines == mNewTimelines)
        {
            return false;
        }
        mOldTimelines = aDocument.timelines;
        aDocument.timelines = mNewTimelines;
        aChanges.timelines = true;
        return true;
    }

    void SetTimelinesCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        aDocument.timelines = mOldTimelines;
        aChanges.timelines = true;
    }

    bool SetTimelinesCommand::canMergeWith(const Command& aNext) const
    {
        const auto* next = dynamic_cast<const SetTimelinesCommand*>(&aNext);
        return next && !mMergeKey.empty() && next->mMergeKey == mMergeKey;
    }

    void SetTimelinesCommand::mergeWith(Command& aNext)
    {
        mNewTimelines = static_cast<SetTimelinesCommand&>(aNext).mNewTimelines;
    }
}
