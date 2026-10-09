#include "doctest.h"

#include "ContentCheck.h"

using namespace hg;

TEST_SUITE("Content")
{
    TEST_CASE("shipped content validates without errors")
    {
        const tool::ContentReport report = tool::checkContent(HG_CONTENT_DIR);
        for (const auto& entry : report.diagnostics.entries())
        {
            INFO(formatDiagnostic(entry));
        }
        CHECK(report.scenes > 0);
        CHECK_FALSE(report.diagnostics.hasErrors());
    }
}
