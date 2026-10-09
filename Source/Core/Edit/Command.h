#pragma once

#include "Core/Base/Ids.h"
#include "Core/Scene/Document.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hg
{
    struct PropertyChange
    {
        ObjectId uid = kInvalidObjectId;
        std::string property;

        bool operator==(const PropertyChange& aOther) const = default;
    };

    struct ChangeSet
    {
        bool structure = false;
        bool settings = false;
        bool timelines = false;
        std::vector<PropertyChange> properties;
        std::vector<ObjectId> objects;

        void merge(const ChangeSet& aOther);
        void addProperty(ObjectId aUid, std::string aProperty);
        void addObject(ObjectId aUid);
        bool empty() const;
    };

    class Command
    {
    public:
        virtual ~Command() = default;

        virtual std::string label() const = 0;
        virtual bool apply(SceneDocument& aDocument, ChangeSet& aChanges) = 0;
        virtual void revert(SceneDocument& aDocument, ChangeSet& aChanges) = 0;

        virtual bool canMergeWith(const Command& aNext) const
        {
            return false;
        }

        virtual void mergeWith(Command& aNext)
        {
        }
    };

    class CompositeCommand : public Command
    {
    public:
        explicit CompositeCommand(std::string aLabel);

        void add(std::unique_ptr<Command> aCommand);
        bool empty() const;

        std::string label() const override;
        bool apply(SceneDocument& aDocument, ChangeSet& aChanges) override;
        void revert(SceneDocument& aDocument, ChangeSet& aChanges) override;
        bool canMergeWith(const Command& aNext) const override;
        void mergeWith(Command& aNext) override;

    private:
        std::string mLabel;
        std::vector<std::unique_ptr<Command>> mCommands;
        size_t mAppliedCount = 0;
    };

    class CommandHistory
    {
    public:
        bool execute(std::unique_ptr<Command> aCommand, SceneDocument& aDocument, ChangeSet& aChanges, bool aAllowMerge);
        bool undo(SceneDocument& aDocument, ChangeSet& aChanges);
        bool redo(SceneDocument& aDocument, ChangeSet& aChanges);

        bool canUndo() const;
        bool canRedo() const;
        std::string undoLabel() const;
        std::string redoLabel() const;

        void closeMergeWindow();
        void clear();
        void markSaved();
        bool isDirty() const;

    private:
        static constexpr size_t kUnreachable = static_cast<size_t>(-1);
        static constexpr size_t kMaxDepth = 512;

        std::vector<std::unique_ptr<Command>> mUndo;
        std::vector<std::unique_ptr<Command>> mRedo;
        size_t mSavedDepth = 0;
        bool mMergeWindowOpen = false;
    };
}
