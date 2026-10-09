#include "ContentCheck.h"
#include "LegacyImport.h"

#include "Core/Base/Json.h"

#include <cstdio>
#include <string>
#include <vector>

namespace
{
    void printUsage()
    {
        std::printf("Usage:\n");
        std::printf("  hg_tool validate <content_dir>\n");
        std::printf("  hg_tool format <file.json> [more files]\n");
        std::printf("  hg_tool import-legacy <legacy_root> <content_dir>\n");
    }

    void printDiagnostics(const hg::Diagnostics& aDiagnostics)
    {
        for (const auto& entry : aDiagnostics.entries())
        {
            std::printf("%s\n", hg::formatDiagnostic(entry).c_str());
        }
    }

    int runValidate(const std::filesystem::path& aContentRoot)
    {
        const hg::tool::ContentReport report = hg::tool::checkContent(aContentRoot);
        printDiagnostics(report.diagnostics);
        std::printf("Checked %zu scenes and %zu prefabs: %zu errors, %zu warnings\n", report.scenes, report.prefabs, report.diagnostics.count(hg::Severity::Error), report.diagnostics.count(hg::Severity::Warning));
        return report.diagnostics.hasErrors() ? 1 : 0;
    }

    int runFormat(const std::vector<std::filesystem::path>& aFiles)
    {
        int failures = 0;
        for (const auto& file : aFiles)
        {
            auto text = hg::tool::readTextFile(file);
            if (!text)
            {
                std::printf("%s: cannot read\n", file.string().c_str());
                ++failures;
                continue;
            }
            auto parsed = hg::parseJson(*text);
            if (!parsed)
            {
                std::printf("%s: %s\n", file.string().c_str(), parsed.error().c_str());
                ++failures;
                continue;
            }
            const std::string formatted = hg::writeJson(parsed.value());
            if (formatted != *text && !hg::tool::writeTextFile(file, formatted))
            {
                std::printf("%s: cannot write\n", file.string().c_str());
                ++failures;
            }
        }
        return failures == 0 ? 0 : 1;
    }

    int runImportLegacy(const std::filesystem::path& aLegacyRoot, const std::filesystem::path& aContentRoot)
    {
        const hg::tool::LegacyImportResult result = hg::tool::importLegacyContent(aLegacyRoot, aContentRoot);
        printDiagnostics(result.diagnostics);
        for (const auto& file : result.writtenFiles)
        {
            std::printf("written %s\n", file.c_str());
        }
        return result.diagnostics.hasErrors() ? 1 : 0;
    }
}

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty())
    {
        printUsage();
        return 2;
    }

    const std::string& command = args.front();
    if (command == "validate" && args.size() == 2)
    {
        return runValidate(args[1]);
    }
    if (command == "format" && args.size() >= 2)
    {
        return runFormat({args.begin() + 1, args.end()});
    }
    if (command == "import-legacy" && args.size() == 3)
    {
        return runImportLegacy(args[1], args[2]);
    }

    printUsage();
    return 2;
}
