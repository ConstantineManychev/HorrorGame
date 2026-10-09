#include "Editor/Panels/Panels.h"

#include "Editor/EditorApp.h"
#include "Editor/EditorGui.h"

#include "ImGui/misc/cpp/imgui_stdlib.h"

#include <cctype>
#include <cstdint>

namespace hg
{
    namespace
    {
        ObjectId gRenameTarget = kInvalidObjectId;
        bool gRenameRequested = false;
        std::string gRenameBuffer;

        std::string toLower(std::string aText)
        {
            for (auto& character : aText)
            {
                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            }
            return aText;
        }

        bool matchesFilter(const ObjectDesc& aObject, const std::string& aFilter)
        {
            if (aFilter.empty() || toLower(aObject.name).find(aFilter) != std::string::npos || toLower(aObject.type).find(aFilter) != std::string::npos)
            {
                return true;
            }
            for (const auto& child : aObject.children)
            {
                if (matchesFilter(child, aFilter))
                {
                    return true;
                }
            }
            return false;
        }

        void requestRename(const ObjectDesc& aObject)
        {
            gRenameTarget = aObject.uid;
            gRenameBuffer = aObject.name;
            gRenameRequested = true;
        }

        void drawObjectContextMenu(EditorApp& aApp, const ObjectDesc& aObject, const ObjectDesc* aParent, size_t aIndex, size_t aSiblingCount)
        {
            if (!ImGui::BeginPopupContextItem())
            {
                return;
            }
            if (!aApp.isSelected(aObject.uid))
            {
                aApp.select(aObject.uid);
            }
            if (ImGui::MenuItem("Rename", "F2"))
            {
                requestRename(aObject);
            }
            if (ImGui::BeginMenu("Create Child"))
            {
                drawCreateMenu(aApp, aObject.uid);
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
            {
                aApp.duplicateSelection();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C"))
            {
                aApp.copySelection();
            }
            if (ImGui::MenuItem("Delete", "Del"))
            {
                aApp.deleteSelection();
            }
            ImGui::Separator();
            const ObjectId parentUid = aParent ? aParent->uid : kInvalidObjectId;
            if (ImGui::MenuItem("Move Up", nullptr, false, aIndex > 0))
            {
                aApp.moveObject(aObject.uid, parentUid, aIndex - 1);
            }
            if (ImGui::MenuItem("Move Down", nullptr, false, aIndex + 1 < aSiblingCount))
            {
                aApp.moveObject(aObject.uid, parentUid, aIndex + 2);
            }
            if (ImGui::MenuItem("Move to Root", nullptr, false, aParent != nullptr))
            {
                aApp.moveObject(aObject.uid, kInvalidObjectId, aApp.document()->objects.size());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Frame", "F"))
            {
                aApp.frameObjects({aObject.uid});
            }
            bool hidden = aApp.isHidden(aObject.uid);
            if (ImGui::MenuItem("Hidden in Editor", nullptr, &hidden))
            {
                aApp.setHidden(aObject.uid, hidden);
            }
            bool locked = aApp.isLocked(aObject.uid);
            if (ImGui::MenuItem("Locked in Viewport", nullptr, &locked))
            {
                aApp.setLocked(aObject.uid, locked);
            }
            ImGui::EndPopup();
        }

        void drawObjectNode(EditorApp& aApp, const ObjectDesc& aObject, const ObjectDesc* aParent, size_t aIndex, size_t aSiblingCount, const std::string& aFilter)
        {
            if (!matchesFilter(aObject, aFilter))
            {
                return;
            }

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
            if (aObject.children.empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            if (aApp.isSelected(aObject.uid))
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            std::string label = aObject.name.empty() ? std::string("<unnamed>") : aObject.name;
            if (!aObject.prefab.empty())
            {
                label += "  [prefab]";
            }
            if (aApp.isHidden(aObject.uid))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::kDimText);
            }
            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(aObject.uid)), flags, "%s", label.c_str());
            if (aApp.isHidden(aObject.uid))
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
            {
                aApp.select(aObject.uid, ImGui::GetIO().KeyCtrl);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                aApp.frameObjects({aObject.uid});
            }
            if (ImGui::BeginDragDropSource())
            {
                const ObjectId uid = aObject.uid;
                ImGui::SetDragDropPayload("HG_OBJECT", &uid, sizeof(uid));
                ImGui::Text("Move %s", aObject.name.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HG_OBJECT"))
                {
                    const ObjectId dragged = *static_cast<const ObjectId*>(payload->Data);
                    aApp.moveObject(dragged, aObject.uid, aObject.children.size());
                }
                ImGui::EndDragDropTarget();
            }
            drawObjectContextMenu(aApp, aObject, aParent, aIndex, aSiblingCount);

            ImGui::SameLine();
            const std::string type = aObject.type.empty() ? aApp.objectType(aObject.uid) : aObject.type;
            ImGui::TextDisabled("%s%s", type.c_str(), aApp.isLocked(aObject.uid) ? "  locked" : "");

            if (open)
            {
                for (size_t index = 0; index < aObject.children.size(); ++index)
                {
                    drawObjectNode(aApp, aObject.children[index], &aObject, index, aObject.children.size(), aFilter);
                }
                ImGui::TreePop();
            }
        }
    }

    void drawHierarchyPanel(EditorApp& aApp)
    {
        if (!ImGui::Begin(PanelNames::kHierarchy))
        {
            ImGui::End();
            return;
        }
        const SceneDocument* document = aApp.document();
        if (!document)
        {
            ImGui::TextDisabled("No scene open");
            ImGui::End();
            return;
        }

        static std::string filter;
        ImGui::SetNextItemWidth(-60.0f);
        ImGui::InputTextWithHint("##filter", "search", &filter);
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            ImGui::OpenPopup("CreateRoot");
        }
        ImGui::SetItemTooltip("Create object");
        if (ImGui::BeginPopup("CreateRoot"))
        {
            drawCreateMenu(aApp, kInvalidObjectId);
            ImGui::EndPopup();
        }
        ImGui::Separator();

        const std::string loweredFilter = toLower(filter);
        const std::vector<ObjectDesc> objects = document->objects;
        ImGui::BeginChild("##tree");
        for (size_t index = 0; index < objects.size(); ++index)
        {
            drawObjectNode(aApp, objects[index], nullptr, index, objects.size(), loweredFilter);
        }

        const ImVec2 available = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("##root_drop", ImVec2(std::max(1.0f, available.x), std::max(40.0f, available.y)));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            aApp.clearSelection();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HG_OBJECT"))
            {
                aApp.moveObject(*static_cast<const ObjectId*>(payload->Data), kInvalidObjectId, objects.size());
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginPopupContextItem("##root_context"))
        {
            drawCreateMenu(aApp, kInvalidObjectId);
            ImGui::EndPopup();
        }
        ImGui::EndChild();

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F2, false))
        {
            if (const ObjectDesc* object = aApp.document()->findObject(aApp.primarySelection()))
            {
                requestRename(*object);
            }
        }

        if (gRenameRequested)
        {
            gRenameRequested = false;
            ImGui::OpenPopup("Rename object");
        }
        if (ImGui::BeginPopupModal("Rename object", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
            }
            const bool submitted = ImGui::InputText("Name", &gRenameBuffer, ImGuiInputTextFlags_EnterReturnsTrue);
            if (submitted || ImGui::Button("OK", ImVec2(100.0f, 0.0f)))
            {
                aApp.renameObject(gRenameTarget, gRenameBuffer);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::End();
    }
}
