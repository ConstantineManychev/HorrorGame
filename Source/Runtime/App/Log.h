#pragma once

#include "Core/Base/Diagnostics.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace hg
{
    struct LogEntry
    {
        Severity severity = Severity::Info;
        std::string text;
    };

    class Log
    {
    public:
        using Listener = std::function<void(const LogEntry&)>;

        static void info(std::string aText);
        static void warning(std::string aText);
        static void error(std::string aText);
        static void write(Severity aSeverity, std::string aText);
        static void diagnostics(const Diagnostics& aDiagnostics);

        static size_t addListener(Listener aListener);
        static void removeListener(size_t aId);
    };
}
