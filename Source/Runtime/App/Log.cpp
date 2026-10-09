#include "Runtime/App/Log.h"

#include "axmol.h"

#include <mutex>
#include <unordered_map>

namespace hg
{
    namespace
    {
        struct LogState
        {
            std::mutex mutex;
            std::unordered_map<size_t, Log::Listener> listeners;
            size_t nextId = 1;
        };

        LogState& state()
        {
            static LogState instance;
            return instance;
        }
    }

    void Log::info(std::string aText)
    {
        write(Severity::Info, std::move(aText));
    }

    void Log::warning(std::string aText)
    {
        write(Severity::Warning, std::move(aText));
    }

    void Log::error(std::string aText)
    {
        write(Severity::Error, std::move(aText));
    }

    void Log::write(Severity aSeverity, std::string aText)
    {
        switch (aSeverity)
        {
        case Severity::Info:
            AXLOGI("{}", aText);
            break;
        case Severity::Warning:
            AXLOGW("{}", aText);
            break;
        case Severity::Error:
            AXLOGE("{}", aText);
            break;
        }

        std::unordered_map<size_t, Listener> listeners;
        {
            std::lock_guard lock(state().mutex);
            listeners = state().listeners;
        }
        const LogEntry entry{aSeverity, std::move(aText)};
        for (auto& [id, listener] : listeners)
        {
            listener(entry);
        }
    }

    void Log::diagnostics(const Diagnostics& aDiagnostics)
    {
        for (const auto& entry : aDiagnostics.entries())
        {
            write(entry.severity, formatDiagnostic(entry));
        }
    }

    size_t Log::addListener(Listener aListener)
    {
        std::lock_guard lock(state().mutex);
        const size_t id = state().nextId++;
        state().listeners.emplace(id, std::move(aListener));
        return id;
    }

    void Log::removeListener(size_t aId)
    {
        std::lock_guard lock(state().mutex);
        state().listeners.erase(aId);
    }
}
