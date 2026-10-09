#pragma once

#include "Core/Edit/Command.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace hg
{
    class EditSession
    {
    public:
        using Listener = std::function<void(const ChangeSet&)>;

        explicit EditSession(SceneDocument aDocument);

        const SceneDocument& document() const;
        SceneDocument& mutableDocument();

        bool execute(std::unique_ptr<Command> aCommand, bool aAllowMerge = false);
        bool undo();
        bool redo();
        void closeMergeWindow();

        bool canUndo() const;
        bool canRedo() const;
        std::string undoLabel() const;
        std::string redoLabel() const;

        void markSaved();
        bool isDirty() const;

        void replaceDocument(SceneDocument aDocument);

        size_t addListener(Listener aListener);
        void removeListener(size_t aId);

    private:
        void notify(const ChangeSet& aChanges);

        SceneDocument mDocument;
        CommandHistory mHistory;
        std::unordered_map<size_t, Listener> mListeners;
        size_t mNextListenerId = 1;
    };
}
