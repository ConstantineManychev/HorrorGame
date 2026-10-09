#pragma once

#include "Core/Base/Math.h"
#include "Core/Base/Value.h"
#include "Core/Scene/Schema.h"

#include "ImGui/imgui.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace hg
{
    class EditorApp;

    namespace EditorColors
    {
        inline constexpr ImU32 kSelection = IM_COL32(255, 170, 40, 255);
        inline constexpr ImU32 kSelectionSecondary = IM_COL32(255, 210, 120, 200);
        inline constexpr ImU32 kAxisX = IM_COL32(235, 70, 70, 255);
        inline constexpr ImU32 kAxisY = IM_COL32(90, 210, 90, 255);
        inline constexpr ImU32 kRotate = IM_COL32(80, 150, 255, 255);
        inline constexpr ImU32 kHighlight = IM_COL32(255, 255, 255, 255);
        inline constexpr ImU32 kPath = IM_COL32(60, 220, 255, 230);
        inline constexpr ImU32 kTrigger = IM_COL32(255, 220, 60, 200);
        inline constexpr ImU32 kTriggerFill = IM_COL32(255, 220, 60, 30);
        inline constexpr ImU32 kCollider = IM_COL32(255, 80, 80, 220);
        inline constexpr ImU32 kColliderOneWay = IM_COL32(255, 150, 40, 220);
        inline constexpr ImU32 kSpawn = IM_COL32(120, 255, 140, 255);
        inline constexpr ImU32 kCameraFrame = IM_COL32(180, 120, 255, 230);
        inline constexpr ImU32 kWorldBounds = IM_COL32(255, 255, 255, 90);
        inline constexpr ImU32 kGrid = IM_COL32(255, 255, 255, 22);
        inline constexpr ImU32 kKey = IM_COL32(230, 230, 230, 255);
        inline constexpr ImU32 kKeySelected = IM_COL32(255, 170, 40, 255);
        inline constexpr ImU32 kPlayhead = IM_COL32(255, 70, 70, 255);
        inline constexpr ImU32 kRecording = IM_COL32(230, 40, 40, 255);
        inline constexpr ImVec4 kWarningText{1.0f, 0.8f, 0.3f, 1.0f};
        inline constexpr ImVec4 kErrorText{1.0f, 0.4f, 0.4f, 1.0f};
        inline constexpr ImVec4 kDimText{0.6f, 0.6f, 0.6f, 1.0f};
        inline constexpr ImVec4 kOverrideText{0.55f, 0.8f, 1.0f, 1.0f};
    }

    Vec2 imguiToScene(const ImVec2& aPoint);
    ImVec2 sceneToImGui(const Vec2& aPoint);
    float sceneToImGuiScale();

    struct PropertyEdit
    {
        std::optional<Value> value;
        bool active = false;
        bool finished = false;
        bool reset = false;

        bool changed() const
        {
            return value.has_value() || reset;
        }
    };

    struct PropertyEditorContext
    {
        EditorApp* app = nullptr;
        bool overridden = false;
        bool allowReset = false;
    };

    PropertyEdit editProperty(const PropertySchema& aProperty, const Value& aValue, PropertyEditorContext& aContext);
    PropertyEdit editValueJson(const char* aLabel, const Value& aValue);
    bool editActionList(const char* aLabel, Value& aActions, PropertyEditorContext& aContext, bool& aActive);

    void helpMarker(const std::string& aText);
    std::string objectLabel(const std::string& aName, uint64_t aUid);
}
