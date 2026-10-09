#pragma once

#include "ImGui/imgui.h"

namespace hg
{
    class EditorApp;

    namespace PanelNames
    {
        inline constexpr const char* kScenes = "Scenes";
        inline constexpr const char* kHierarchy = "Hierarchy";
        inline constexpr const char* kInspector = "Inspector";
        inline constexpr const char* kViewport = "Viewport";
        inline constexpr const char* kTimeline = "Timeline";
        inline constexpr const char* kAssets = "Assets";
        inline constexpr const char* kConsole = "Console";
    }

    void buildDefaultLayout(ImGuiID aDockspace, bool aReplaceExisting);
    void drawMainMenu(EditorApp& aApp);
    void drawScenesPanel(EditorApp& aApp);
    void drawHierarchyPanel(EditorApp& aApp);
    void drawInspectorPanel(EditorApp& aApp);
    void drawViewportPanel(EditorApp& aApp);
    void drawTimelinePanel(EditorApp& aApp);
    void drawAssetsPanel(EditorApp& aApp);
    void drawConsolePanel(EditorApp& aApp);
    void drawModals(EditorApp& aApp);
    void handleShortcuts(EditorApp& aApp);
    void openNewScenePopup();
    void drawCreateMenu(EditorApp& aApp, unsigned long long aParent);
}
