#pragma once

#include <string>
#include <vector>

namespace hg
{
    enum class Severity
    {
        Info,
        Warning,
        Error
    };

    struct Diagnostic
    {
        Severity severity = Severity::Info;
        std::string source;
        std::string path;
        std::string message;
    };

    class Diagnostics
    {
    public:
        void setSource(std::string aSource);
        const std::string& getSource() const;

        void report(Severity aSeverity, std::string aPath, std::string aMessage);
        void info(std::string aPath, std::string aMessage);
        void warning(std::string aPath, std::string aMessage);
        void error(std::string aPath, std::string aMessage);

        void append(const Diagnostics& aOther);
        void clear();

        const std::vector<Diagnostic>& entries() const;
        bool empty() const;
        bool hasErrors() const;
        size_t count(Severity aSeverity) const;

    private:
        std::string mSource;
        std::vector<Diagnostic> mEntries;
    };

    std::string_view severityName(Severity aSeverity);
    std::string formatDiagnostic(const Diagnostic& aDiagnostic);
}
