#pragma once

#include "Core/Edit/Command.h"

#include <optional>

namespace hg
{
    class SetPropertyCommand : public Command
    {
    public:
        SetPropertyCommand(ObjectId aUid, std::string aProperty, std::optional<Value> aValue);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;
        bool canMergeWith(const Command& aNext) const override;
        void mergeWith(Command& aNext) override;

    private:
        void assign(SceneDocument& aDocument, const std::optional<Value>& aValue, ChangeSet& aChanges);

        ObjectId mUid;
        std::string mProperty;
        std::optional<Value> mNewValue;
        std::optional<Value> mOldValue;
    };

    class RenameObjectCommand : public Command
    {
    public:
        RenameObjectCommand(ObjectId aUid, std::string aName);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;

    private:
        ObjectId mUid;
        std::string mNewName;
        std::string mOldName;
    };

    class CreateObjectCommand : public Command
    {
    public:
        CreateObjectCommand(ObjectId aParentUid, size_t aIndex, ObjectDesc aObject);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;

        ObjectId createdUid() const;

    private:
        ObjectId mParentUid;
        size_t mIndex;
        ObjectDesc mObject;
    };

    class DeleteObjectCommand : public Command
    {
    public:
        explicit DeleteObjectCommand(ObjectId aUid);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;

    private:
        ObjectId mUid;
        ObjectId mParentUid = kInvalidObjectId;
        size_t mIndex = 0;
        ObjectDesc mObject;
    };

    class MoveObjectCommand : public Command
    {
    public:
        MoveObjectCommand(ObjectId aUid, ObjectId aNewParentUid, size_t aNewIndex);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;

    private:
        static bool move(SceneDocument& aDocument, ObjectId aUid, ObjectId aParentUid, size_t aFinalIndex);

        ObjectId mUid;
        ObjectId mNewParentUid;
        size_t mNewIndex;
        ObjectId mOldParentUid = kInvalidObjectId;
        size_t mOldIndex = 0;
    };

    class AddComponentCommand : public Command
    {
    public:
        AddComponentCommand(ObjectId aUid, ComponentDesc aComponent);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;

    private:
        ObjectId mUid;
        ComponentDesc mComponent;
    };

    class RemoveComponentCommand : public Command
    {
    public:
        RemoveComponentCommand(ObjectId aUid, size_t aIndex);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;

    private:
        ObjectId mUid;
        size_t mIndex;
        ComponentDesc mComponent;
    };

    class SetComponentPropertyCommand : public Command
    {
    public:
        SetComponentPropertyCommand(ObjectId aUid, size_t aComponentIndex, std::string aProperty, std::optional<Value> aValue);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;
        bool canMergeWith(const Command& aNext) const override;
        void mergeWith(Command& aNext) override;

    private:
        bool assign(SceneDocument& aDocument, const std::optional<Value>& aValue, ChangeSet& aChanges);

        ObjectId mUid;
        size_t mComponentIndex;
        std::string mProperty;
        std::optional<Value> mNewValue;
        std::optional<Value> mOldValue;
    };

    class SetSettingCommand : public Command
    {
    public:
        SetSettingCommand(std::string aKey, std::optional<Value> aValue);

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;
        bool canMergeWith(const Command& aNext) const override;
        void mergeWith(Command& aNext) override;

    private:
        std::string mKey;
        std::optional<Value> mNewValue;
        std::optional<Value> mOldValue;
    };

    class SetTimelinesCommand : public Command
    {
    public:
        SetTimelinesCommand(std::vector<TimelineDesc> aTimelines, std::string aLabel, std::string aMergeKey = {});

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;
        bool canMergeWith(const Command& aNext) const override;
        void mergeWith(Command& aNext) override;

    private:
        std::vector<TimelineDesc> mNewTimelines;
        std::vector<TimelineDesc> mOldTimelines;
        std::string mLabel;
        std::string mMergeKey;
    };
}
