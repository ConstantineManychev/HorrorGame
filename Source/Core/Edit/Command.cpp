#include "Core/Edit/Command.h"

#include <algorithm>

namespace hg
{
    void ChangeSet::merge(const ChangeSet& aOther)
    {
        structure = structure || aOther.structure;
        settings = settings || aOther.settings;
        timelines = timelines || aOther.timelines;
        for (const auto& property : aOther.properties)
        {
            addProperty(property.uid, property.property);
        }
        for (ObjectId uid : aOther.objects)
        {
            addObject(uid);
        }
    }

    void ChangeSet::addProperty(ObjectId aUid, std::string aProperty)
    {
        PropertyChange change{aUid, std::move(aProperty)};
        if (std::find(properties.begin(), properties.end(), change) == properties.end())
        {
            properties.push_back(std::move(change));
        }
    }

    void ChangeSet::addObject(ObjectId aUid)
    {
        if (std::find(objects.begin(), objects.end(), aUid) == objects.end())
        {
            objects.push_back(aUid);
        }
    }

    bool ChangeSet::empty() const
    {
        return !structure && !settings && !timelines && properties.empty() && objects.empty();
    }

    CompositeCommand::CompositeCommand(std::string aLabel)
        : mLabel(std::move(aLabel))
    {
    }

    void CompositeCommand::add(std::unique_ptr<Command> aCommand)
    {
        if (aCommand)
        {
            mCommands.push_back(std::move(aCommand));
        }
    }

    bool CompositeCommand::empty() const
    {
        return mCommands.empty();
    }

    std::string CompositeCommand::label() const
    {
        return mLabel;
    }

    bool CompositeCommand::apply(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        mAppliedCount = 0;
        for (auto& command : mCommands)
        {
            if (!command->apply(aDocument, aChanges))
            {
                for (size_t index = mAppliedCount; index > 0; --index)
                {
                    mCommands[index - 1]->revert(aDocument, aChanges);
                }
                mAppliedCount = 0;
                return false;
            }
            ++mAppliedCount;
        }
        return mAppliedCount > 0;
    }

    void CompositeCommand::revert(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        for (size_t index = mAppliedCount; index > 0; --index)
        {
            mCommands[index - 1]->revert(aDocument, aChanges);
        }
    }

    bool CompositeCommand::canMergeWith(const Command& aNext) const
    {
        const auto* next = dynamic_cast<const CompositeCommand*>(&aNext);
        if (!next || next->mCommands.size() != mCommands.size() || next->mLabel != mLabel)
        {
            return false;
        }
        for (size_t index = 0; index < mCommands.size(); ++index)
        {
            if (!mCommands[index]->canMergeWith(*next->mCommands[index]))
            {
                return false;
            }
        }
        return true;
    }

    void CompositeCommand::mergeWith(Command& aNext)
    {
        auto& next = static_cast<CompositeCommand&>(aNext);
        for (size_t index = 0; index < mCommands.size(); ++index)
        {
            mCommands[index]->mergeWith(*next.mCommands[index]);
        }
    }

    bool CommandHistory::execute(std::unique_ptr<Command> aCommand, SceneDocument& aDocument, ChangeSet& aChanges, bool aAllowMerge)
    {
        if (!aCommand || !aCommand->apply(aDocument, aChanges))
        {
            return false;
        }

        if (mSavedDepth > mUndo.size() && mSavedDepth != kUnreachable)
        {
            mSavedDepth = kUnreachable;
        }
        mRedo.clear();

        if (aAllowMerge && mMergeWindowOpen && !mUndo.empty() && mUndo.back()->canMergeWith(*aCommand))
        {
            mUndo.back()->mergeWith(*aCommand);
            if (mSavedDepth == mUndo.size())
            {
                mSavedDepth = kUnreachable;
            }
            return true;
        }

        mUndo.push_back(std::move(aCommand));
        mMergeWindowOpen = aAllowMerge;

        if (mUndo.size() > kMaxDepth)
        {
            mUndo.erase(mUndo.begin());
            if (mSavedDepth != kUnreachable)
            {
                mSavedDepth = mSavedDepth == 0 ? kUnreachable : mSavedDepth - 1;
            }
        }
        return true;
    }

    bool CommandHistory::undo(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (mUndo.empty())
        {
            return false;
        }
        mMergeWindowOpen = false;
        auto command = std::move(mUndo.back());
        mUndo.pop_back();
        command->revert(aDocument, aChanges);
        mRedo.push_back(std::move(command));
        return true;
    }

    bool CommandHistory::redo(SceneDocument& aDocument, ChangeSet& aChanges)
    {
        if (mRedo.empty())
        {
            return false;
        }
        mMergeWindowOpen = false;
        auto command = std::move(mRedo.back());
        mRedo.pop_back();
        if (!command->apply(aDocument, aChanges))
        {
            mRedo.clear();
            return false;
        }
        mUndo.push_back(std::move(command));
        return true;
    }

    bool CommandHistory::canUndo() const
    {
        return !mUndo.empty();
    }

    bool CommandHistory::canRedo() const
    {
        return !mRedo.empty();
    }

    std::string CommandHistory::undoLabel() const
    {
        return mUndo.empty() ? std::string() : mUndo.back()->label();
    }

    std::string CommandHistory::redoLabel() const
    {
        return mRedo.empty() ? std::string() : mRedo.back()->label();
    }

    void CommandHistory::closeMergeWindow()
    {
        mMergeWindowOpen = false;
    }

    void CommandHistory::clear()
    {
        mUndo.clear();
        mRedo.clear();
        mSavedDepth = 0;
        mMergeWindowOpen = false;
    }

    void CommandHistory::markSaved()
    {
        mSavedDepth = mUndo.size();
        mMergeWindowOpen = false;
    }

    bool CommandHistory::isDirty() const
    {
        return mSavedDepth != mUndo.size();
    }
}
