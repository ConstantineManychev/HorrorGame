#include "Editor/Panels/Panels.h"

#include "Editor/EditorApp.h"
#include "Editor/EditorGui.h"
#include "Runtime/App/AppContext.h"

#include "ImGui/ImGuiPresenter.h"
#include "ImGui/misc/cpp/imgui_stdlib.h"

namespace hg
{
    namespace
    {
        bool isImage(const std::string& aPath)
        {
            for (const char* extension : {".png", ".jpg", ".jpeg", ".webp"})
            {
                const std::string suffix(extension);
                if (aPath.size() >= suffix.size() && aPath.compare(aPath.size() - suffix.size(), suffix.size(), suffix) == 0)
                {
                    return true;
                }
            }
            return false;
        }

        void imageTooltip(const std::string& aPath)
        {
            if (!isImage(aPath) || !ImGui::BeginItemTooltip())
            {
                return;
            }
            if (auto* texture = ax::Director::getInstance()->getTextureCache()->addImage(aPath))
            {
                const ax::Size size = texture->getContentSize();
                const float scale = std::min(1.0f, 220.0f / std::max(size.width, size.height));
                ax::extension::ImGuiPresenter::getInstance()->image(texture, ImVec2(size.width * scale, size.height * scale));
                ImGui::Text("%.0f x %.0f", size.width, size.height);
            }
            ImGui::EndTooltip();
        }

        void dragSource(const char* aType, const std::string& aValue)
        {
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(aType, aValue.data(), aValue.size());
                ImGui::TextUnformatted(aValue.c_str());
                ImGui::EndDragDropSource();
            }
        }

        bool passes(const std::string& aValue, const std::string& aFilter)
        {
            return aFilter.empty() || aValue.find(aFilter) != std::string::npos;
        }
    }

    void drawAssetsPanel(EditorApp& aApp)
    {
        if (!ImGui::Begin(PanelNames::kAssets))
        {
            ImGui::End();
            return;
        }
        static std::string filter;
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputTextWithHint("##filter", "filter", &filter);
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            aApp.refreshContentLists();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Drag items into the viewport or onto property fields");

        if (ImGui::BeginTabBar("##asset_tabs"))
        {
            if (ImGui::BeginTabItem("Files"))
            {
                ImGui::BeginChild("##files");
                for (const auto& file : aApp.assetFiles())
                {
                    if (!passes(file, filter))
                    {
                        continue;
                    }
                    ImGui::Selectable(file.c_str());
                    dragSource("HG_ASSET", file);
                    imageTooltip(file);
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Atlas frames"))
            {
                ImGui::BeginChild("##frames");
                for (const auto& frame : aApp.atlasFrames())
                {
                    if (!passes(frame, filter))
                    {
                        continue;
                    }
                    ImGui::Selectable(frame.c_str());
                    dragSource("HG_FRAME", frame);
                    if (ImGui::BeginItemTooltip())
                    {
                        if (auto* spriteFrame = ax::SpriteFrameCache::getInstance()->findFrame(frame))
                        {
                            ax::extension::ImGuiPresenter::getInstance()->image(spriteFrame, ImVec2(160.0f, 160.0f));
                        }
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Prefabs"))
            {
                ImGui::BeginChild("##prefabs");
                for (const auto& prefab : aApp.prefabFiles())
                {
                    if (!passes(prefab, filter))
                    {
                        continue;
                    }
                    if (ImGui::Selectable(prefab.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        aApp.createPrefabInstance(prefab, kInvalidObjectId, aApp.viewportCenterWorld());
                    }
                    dragSource("HG_ASSET", prefab);
                    ImGui::SetItemTooltip("Double-click or drag into the viewport to place");
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    void drawConsolePanel(EditorApp& aApp)
    {
        if (!ImGui::Begin(PanelNames::kConsole))
        {
            ImGui::End();
            return;
        }
        static bool showInfo = true;
        static bool showWarnings = true;
        static bool showErrors = true;
        if (ImGui::Button("Clear"))
        {
            aApp.clearLog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Validate scene"))
        {
            aApp.validate();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Info", &showInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warnings", &showWarnings);
        ImGui::SameLine();
        ImGui::Checkbox("Errors", &showErrors);
        ImGui::Separator();

        ImGui::BeginChild("##log", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& entry : aApp.logEntries())
        {
            if ((entry.severity == Severity::Info && !showInfo) || (entry.severity == Severity::Warning && !showWarnings) || (entry.severity == Severity::Error && !showErrors))
            {
                continue;
            }
            if (entry.severity == Severity::Error)
            {
                ImGui::TextColored(EditorColors::kErrorText, "%s", entry.text.c_str());
            }
            else if (entry.severity == Severity::Warning)
            {
                ImGui::TextColored(EditorColors::kWarningText, "%s", entry.text.c_str());
            }
            else
            {
                ImGui::TextUnformatted(entry.text.c_str());
            }
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
