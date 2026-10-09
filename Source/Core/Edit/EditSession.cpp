#include "Core/Edit/EditSession.h"

namespace hg
{
    EditSession::EditSession(SceneDocument aDocument)
        : mDocument(std::move(aDocument))
    {
    }

    const SceneDocument& EditSession::document() const
    {
        return mDocument;
    }

    SceneDocument& EditSession::mutableDocument()
    {
        return mDocument;
    }

    bool EditSession::execute(std::unique_ptr<Command> aCommand, bool aAllowMerge)
    {
        ChangeSet changes;
        if (!mHistory.execute(std::move(aCommand), mDocument, changes, aAllowMerge))
        {
            return false;
        }
        notify(changes);
        return true;
    }

    bool EditSession::undo()
    {
        ChangeSet changes;
        if (!mHistory.undo(mDocument, changes))
        {
            return false;
        }
        notify(changes);
        return true;
    }

    bool EditSession::redo()
    {
        ChangeSet changes;
        if (!mHistory.redo(mDocument, changes))
        {
            return false;
        }
        notify(changes);
        return true;
    }

    void EditSession::closeMergeWindow()
    {
        mHistory.closeMergeWindow();
    }

    bool EditSession::canUndo() const
    {
        return mHistory.canUndo();
    }

    bool EditSession::canRedo() const
    {
        return mHistory.canRedo();
    }

    std::string EditSession::undoLabel() const
    {
        return mHistory.undoLabel();
    }

    std::string EditSession::redoLabel() const
    {
        return mHistory.redoLabel();
    }

    void EditSession::markSaved()
    {
        mHistory.markSaved();
    }

    bool EditSession::isDirty() const
    {
        return mHistory.isDirty();
    }

    void EditSession::replaceDocument(SceneDocument aDocument)
    {
        mDocument = std::move(aDocument);
        mHistory.clear();
        ChangeSet changes;
        changes.structure = true;
        changes.settings = true;
        changes.timelines = true;
        notify(changes);
    }

    size_t EditSession::addListener(Listener aListener)
    {
        const size_t id = mNextListenerId++;
        mListeners.emplace(id, std::move(aListener));
        return id;
    }

    void EditSession::removeListener(size_t aId)
    {
        mListeners.erase(aId);
    }

    void EditSession::notify(const ChangeSet& aChanges)
    {
        if (aChanges.empty())
        {
            return;
        }
        auto listeners = mListeners;
        for (auto& [id, listener] : listeners)
        {
            listener(aChanges);
        }
    }
}
