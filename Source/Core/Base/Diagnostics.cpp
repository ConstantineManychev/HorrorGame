#include "Core/Base/Diagnostics.h"

#include <algorithm>

namespace hg
{
    void Diagnostics::setSource(std::string aSource)
    {
        mSource = std::move(aSource);
    }

    const std::string& Diagnostics::getSource() const
    {
        return mSource;
    }

    void Diagnostics::report(Severity aSeverity, std::string aPath, std::string aMessage)
    {
        mEntries.push_back(Diagnostic{aSeverity, mSource, std::move(aPath), std::move(aMessage)});
    }

    void Diagnostics::info(std::string aPath, std::string aMessage)
    {
        report(Severity::Info, std::move(aPath), std::move(aMessage));
    }

    void Diagnostics::warning(std::string aPath, std::string aMessage)
    {
        report(Severity::Warning, std::move(aPath), std::move(aMessage));
    }

    void Diagnostics::error(std::string aPath, std::string aMessage)
    {
        report(Severity::Error, std::move(aPath), std::move(aMessage));
    }

    void Diagnostics::append(const Diagnostics& aOther)
    {
        mEntries.insert(mEntries.end(), aOther.mEntries.begin(), aOther.mEntries.end());
    }

    void Diagnostics::clear()
    {
        mEntries.clear();
    }

    const std::vector<Diagnostic>& Diagnostics::entries() const
    {
        return mEntries;
    }

    bool Diagnostics::empty() const
    {
        return mEntries.empty();
    }

    bool Diagnostics::hasErrors() const
    {
        return count(Severity::Error) > 0;
    }

    size_t Diagnostics::count(Severity aSeverity) const
    {
        return static_cast<size_t>(std::count_if(mEntries.begin(), mEntries.end(), [aSeverity](const Diagnostic& aEntry)
        {
            return aEntry.severity == aSeverity;
        }));
    }

    std::string_view severityName(Severity aSeverity)
    {
        switch (aSeverity)
        {
        case Severity::Info:
            return "info";
        case Severity::Warning:
            return "warning";
        case Severity::Error:
            return "error";
        }
        return "unknown";
    }

    std::string formatDiagnostic(const Diagnostic& aDiagnostic)
    {
        std::string text;
        if (!aDiagnostic.source.empty())
        {
            text += aDiagnostic.source;
            text += ": ";
        }
        text += severityName(aDiagnostic.severity);
        if (!aDiagnostic.path.empty())
        {
            text += " at ";
            text += aDiagnostic.path;
        }
        text += ": ";
        text += aDiagnostic.message;
        return text;
    }
}
