#include "Editor/Panels/Panels.h"

#include "Editor/EditorApp.h"
#include "Runtime/App/AppContext.h"

#include "ImGui/imgui_internal.h"
#include "ImGui/misc/cpp/imgui_stdlib.h"

#include <map>

namespace hg
{
    namespace
    {
        bool gNewSceneRequested = false;
        bool gDeleteSceneRequested = false;
        std::string gDeleteSceneId;

        void openScenePopupMenu(EditorApp& aApp)
        {
            for (const auto& id : aApp.sceneIds())
            {
                const bool current = aApp.document() && aApp.document()->id == id;
                if (ImGui::MenuItem(id.c_str(), nullptr, current))
                {
                    aApp.requestOpenScene(id);
                }
            }
        }
    }

    void openNewScenePopup()
    {
        gNewSceneRequested = true;
    }

    void buildDefaultLayout(ImGuiID aDockspace, bool aReplaceExisting)
    {
        ImGuiDockNode* existing = ImGui::DockBuilderGetNode(aDockspace);
        if (!aReplaceExisting && existing && !existing->IsLeafNode())
        {
            return;
        }
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockBuilderRemoveNode(aDockspace);
        ImGui::DockBuilderAddNode(aDockspace, static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_DockSpace) | ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockBuilderSetNodeSize(aDockspace, viewport->WorkSize);

        ImGuiID center = aDockspace;
        const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
        const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, nullptr, &center);
        const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.32f, nullptr, &center);
        ImGuiID leftBottom = 0;
        const ImGuiID leftTop = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.3f, nullptr, &leftBottom);

        ImGui::DockBuilderDockWindow(PanelNames::kScenes, leftTop);
        ImGui::DockBuilderDockWindow(PanelNames::kHierarchy, leftBottom);
        ImGui::DockBuilderDockWindow(PanelNames::kInspector, right);
        ImGui::DockBuilderDockWindow(PanelNames::kAssets, bottom);
        ImGui::DockBuilderDockWindow(PanelNames::kConsole, bottom);
        ImGui::DockBuilderDockWindow(PanelNames::kTimeline, bottom);
        ImGui::DockBuilderFinish(aDockspace);
    }

    void drawCreateMenu(EditorApp& aApp, unsigned long long aParent)
    {
        std::map<std::string, std::vector<const TypeSchema*>> categories;
        for (const auto& type : aApp.schema().types())
        {
            if (type.creatable)
            {
                categories[type.category].push_back(&type);
            }
        }
        const std::optional<Vec2> position = aParent == kInvalidObjectId ? std::optional<Vec2>(aApp.viewportCenterWorld()) : std::nullopt;
        for (const auto& [category, types] : categories)
        {
            if (ImGui::BeginMenu(category.c_str()))
            {
                for (const TypeSchema* type : types)
                {
                    if (ImGui::MenuItem(type->id.c_str()))
                    {
                        aApp.createObject(type->id, aParent, position);
                    }
                    if (!type->description.empty())
                    {
                        ImGui::SetItemTooltip("%s", type->description.c_str());
                    }
                }
                ImGui::EndMenu();
            }
        }
        if (!aApp.prefabFiles().empty() && ImGui::BeginMenu("Prefab"))
        {
            for (const auto& prefab : aApp.prefabFiles())
            {
                if (ImGui::MenuItem(prefab.c_str()))
                {
                    aApp.createPrefabInstance(prefab, aParent, position);
                }
            }
            ImGui::EndMenu();
        }
    }

    void drawMainMenu(EditorApp& aApp)
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }
        EditSession* session = aApp.session();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene..."))
            {
                openNewScenePopup();
            }
            if (ImGui::BeginMenu("Open Scene"))
            {
                openScenePopupMenu(aApp);
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S", false, session != nullptr))
            {
                aApp.saveScene();
            }
            if (ImGui::MenuItem("Revert to Saved", nullptr, false, aApp.isDirty()))
            {
                aApp.reloadScene();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Validate Scene"))
            {
                aApp.validate();
            }
            if (ImGui::MenuItem("Refresh Content"))
            {
                aApp.refreshContentLists();
                aApp.context().content().prefabs().invalidate();
                aApp.reloadScene();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                ax::Director::getInstance()->end();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            const std::string undoLabel = session && session->canUndo() ? "Undo " + session->undoLabel() : std::string("Undo");
            const std::string redoLabel = session && session->canRedo() ? "Redo " + session->redoLabel() : std::string("Redo");
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, session && session->canUndo()))
            {
                aApp.undo();
            }
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, session && session->canRedo()))
            {
                aApp.redo();
            }
            ImGui::Separator();
            const bool hasSelection = !aApp.selection().empty();
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection))
            {
                aApp.copySelection();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V"))
            {
                aApp.paste();
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection))
            {
                aApp.duplicateSelection();
            }
            if (ImGui::MenuItem("Delete", "Del", false, hasSelection))
            {
                aApp.deleteSelection();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Create"))
        {
            drawCreateMenu(aApp, aApp.primarySelection() != kInvalidObjectId && ImGui::GetIO().KeyShift ? aApp.primarySelection() : kInvalidObjectId);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ViewportState& viewport = aApp.viewport();
            ImGui::MenuItem("Grid", nullptr, &viewport.showGrid);
            ImGui::MenuItem("Helpers", nullptr, &viewport.showHelpers);
            ImGui::MenuItem("Game Camera Frame", nullptr, &viewport.showCameraFrame);
            if (ImGui::MenuItem("Parallax Preview", nullptr, &viewport.parallax))
            {
                aApp.view().setParallaxEnabled(viewport.parallax);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Frame All", "Home"))
            {
                aApp.frameAll();
            }
            if (ImGui::MenuItem("Frame Selection", "F", false, !aApp.selection().empty()))
            {
                aApp.frameObjects(aApp.selection());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout"))
            {
                aApp.resetLayout();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Play"))
        {
            if (ImGui::MenuItem("Play Scene", "Ctrl+P", false, !aApp.isPlaying()))
            {
                aApp.startPlay(std::nullopt);
            }
            if (ImGui::MenuItem("Play From Viewport Center", "Ctrl+Shift+P", false, !aApp.isPlaying()))
            {
                aApp.startPlay(aApp.viewportCenterWorld());
            }
            ImGui::MenuItem("Debug Draw in Play Mode", nullptr, &aApp.playDebugDraw());
            ImGui::EndMenu();
        }

        if (const SceneDocument* document = aApp.document())
        {
            const std::string title = document->id + (aApp.isDirty() ? " *" : "") + "  (" + std::string(sceneKindName(document->kind)) + ")";
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(title.c_str()).x - 20.0f);
            ImGui::TextDisabled("%s", title.c_str());
        }
        ImGui::EndMainMenuBar();
    }

    void drawScenesPanel(EditorApp& aApp)
    {
        if (!ImGui::Begin(PanelNames::kScenes))
        {
            ImGui::End();
            return;
        }
        if (ImGui::Button("New"))
        {
            openNewScenePopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            aApp.saveScene();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            aApp.refreshContentLists();
        }
        ImGui::Separator();
        const SceneDocument* document = aApp.document();
        for (const auto& id : aApp.sceneIds())
        {
            const bool current = document && document->id == id;
            const std::string label = id + (current && aApp.isDirty() ? " *" : "");
            if (ImGui::Selectable(label.c_str(), current, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                aApp.requestOpenScene(id);
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Open"))
                {
                    aApp.requestOpenScene(id);
                }
                if (ImGui::MenuItem("Delete..."))
                {
                    gDeleteSceneRequested = true;
                    gDeleteSceneId = id;
                }
                ImGui::EndPopup();
            }
        }
        ImGui::TextDisabled("Double-click to open, right-click for more");
        ImGui::End();
    }

    void handleShortcuts(EditorApp& aApp)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput || aApp.isPlaying())
        {
            return;
        }
        const bool ctrl = io.KeyCtrl || io.KeySuper;
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            aApp.saveScene();
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
        {
            if (io.KeyShift)
            {
                aApp.redo();
            }
            else
            {
                aApp.undo();
            }
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
        {
            aApp.redo();
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
        {
            aApp.copySelection();
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V, false))
        {
            aApp.paste();
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
        {
            aApp.duplicateSelection();
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_P, false))
        {
            aApp.startPlay(io.KeyShift ? std::optional<Vec2>(aApp.viewportCenterWorld()) : std::nullopt);
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        {
            aApp.deleteSelection();
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
            aApp.frameObjects(aApp.selection());
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_Home, false))
        {
            aApp.frameAll();
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_W, false))
        {
            aApp.viewport().gizmo = GizmoMode::Move;
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_E, false))
        {
            aApp.viewport().gizmo = GizmoMode::Rotate;
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_R, false))
        {
            aApp.viewport().gizmo = GizmoMode::Scale;
        }
    }

    void drawModals(EditorApp& aApp)
    {
        if (gNewSceneRequested)
        {
            gNewSceneRequested = false;
            ImGui::OpenPopup("New scene");
        }
        if (gDeleteSceneRequested)
        {
            gDeleteSceneRequested = false;
            ImGui::OpenPopup("Delete scene");
        }

        PendingAction& pending = aApp.pendingAction();
        if (pending.kind != PendingAction::Kind::None && !ImGui::IsPopupOpen("Unsaved changes"))
        {
            ImGui::OpenPopup("Unsaved changes");
        }

        if (ImGui::BeginPopupModal("New scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static std::string id = "new_scene";
            static int kind = 0;
            ImGui::InputText("Id", &id);
            ImGui::RadioButton("Location", &kind, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Screen (menu, UI)", &kind, 1);
            ImGui::TextDisabled("Lowercase letters, digits and '_'");
            if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
            {
                aApp.requestNewScene(id, kind == 0 ? SceneKind::Location : SceneKind::Screen);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Delete scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Delete scene '%s' from disk?", gDeleteSceneId.c_str());
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "This cannot be undone in the editor.");
            if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f)))
            {
                aApp.deleteScene(gDeleteSceneId);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Scene '%s' has unsaved changes.", aApp.document() ? aApp.document()->id.c_str() : "");
            auto proceed = [&aApp, &pending]()
            {
                const PendingAction action = pending;
                pending = {};
                if (action.kind == PendingAction::Kind::OpenScene)
                {
                    aApp.openScene(action.sceneId);
                }
                else if (action.kind == PendingAction::Kind::NewScene)
                {
                    aApp.createScene(action.sceneId, action.sceneKind);
                }
            };
            if (ImGui::Button("Save", ImVec2(110.0f, 0.0f)))
            {
                if (aApp.saveScene())
                {
                    proceed();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard", ImVec2(110.0f, 0.0f)))
            {
                if (EditSession* session = aApp.session())
                {
                    session->markSaved();
                }
                proceed();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                pending = {};
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}
