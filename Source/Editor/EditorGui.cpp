#include "Editor/EditorGui.h"

#include "Core/Base/Json.h"
#include "Core/Base/ValueConvert.h"
#include "Editor/EditorApp.h"

#include "ImGui/misc/cpp/imgui_stdlib.h"
#include "axmol.h"

#include <algorithm>
#include <cmath>

namespace hg
{
    namespace
    {
        constexpr float kLabelWidth = 130.0f;

        struct FrameMapping
        {
            ax::Vec2 frame;
            ax::Rect viewport;
            float scaleX = 1.0f;
            float scaleY = 1.0f;
            ImVec2 display;
        };

        FrameMapping frameMapping()
        {
            FrameMapping mapping;
            auto* renderView = ax::Director::getInstance()->getRenderView();
            mapping.frame = renderView->getFrameSize();
            mapping.viewport = renderView->getViewPortRect();
            mapping.scaleX = renderView->getScaleX();
            mapping.scaleY = renderView->getScaleY();
            mapping.display = ImGui::GetCurrentContext() ? ImGui::GetIO().DisplaySize : ImVec2(mapping.frame.x, mapping.frame.y);
            if (mapping.display.x <= 0.0f || mapping.display.y <= 0.0f)
            {
                mapping.display = ImVec2(mapping.frame.x, mapping.frame.y);
            }
            return mapping;
        }

        void beginRow(const PropertySchema& aProperty, const PropertyEditorContext& aContext)
        {
            ImGui::AlignTextToFramePadding();
            if (aContext.overridden)
            {
                ImGui::TextColored(EditorColors::kOverrideText, "%s", aProperty.label.c_str());
            }
            else
            {
                ImGui::TextUnformatted(aProperty.label.c_str());
            }
            if (!aProperty.tooltip.empty())
            {
                ImGui::SetItemTooltip("%s", aProperty.tooltip.c_str());
            }
            ImGui::SameLine(kLabelWidth);
            ImGui::SetNextItemWidth(aContext.allowReset ? -28.0f : -FLT_MIN);
        }

        void trackActivity(PropertyEdit& aEdit)
        {
            aEdit.active = aEdit.active || ImGui::IsItemActive();
            aEdit.finished = aEdit.finished || ImGui::IsItemDeactivatedAfterEdit();
        }

        void acceptDropPayload(const char* aType, std::string& aTarget, PropertyEdit& aEdit)
        {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(aType))
                {
                    aTarget.assign(static_cast<const char*>(payload->Data), static_cast<size_t>(payload->DataSize));
                    aEdit.value = Value(aTarget);
                    aEdit.finished = true;
                }
                ImGui::EndDragDropTarget();
            }
        }

        bool matchesExtension(const std::string& aPath, const std::vector<std::string>& aExtensions)
        {
            if (aExtensions.empty())
            {
                return true;
            }
            for (const auto& extension : aExtensions)
            {
                if (aPath.size() >= extension.size() && aPath.compare(aPath.size() - extension.size(), extension.size(), extension) == 0)
                {
                    return true;
                }
            }
            return false;
        }

        void stringPicker(const char* aPopupId, const std::vector<std::string>& aItems, const std::vector<std::string>& aExtensions, std::string& aCurrent, PropertyEdit& aEdit)
        {
            ImGui::SameLine(0.0f, 2.0f);
            if (ImGui::SmallButton("..."))
            {
                ImGui::OpenPopup(aPopupId);
            }
            if (ImGui::BeginPopup(aPopupId))
            {
                static std::string filter;
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputTextWithHint("##filter", "filter", &filter);
                ImGui::BeginChild("##items", ImVec2(360.0f, 260.0f));
                for (const auto& item : aItems)
                {
                    if (!matchesExtension(item, aExtensions) || (!filter.empty() && item.find(filter) == std::string::npos))
                    {
                        continue;
                    }
                    if (ImGui::Selectable(item.c_str(), item == aCurrent))
                    {
                        aCurrent = item;
                        aEdit.value = Value(item);
                        aEdit.finished = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
        }

        void collectObjects(const std::vector<ObjectDesc>& aObjects, std::vector<const ObjectDesc*>& aOut)
        {
            for (const auto& object : aObjects)
            {
                aOut.push_back(&object);
                collectObjects(object.children, aOut);
            }
        }

        bool editPointList(Value& aPoints, PropertyEdit& aEdit)
        {
            ValueArray points = aPoints.asArray();
            bool modified = false;
            int removeIndex = -1;
            for (size_t index = 0; index < points.size(); ++index)
            {
                ImGui::PushID(static_cast<int>(index));
                Vec2 point = readVec2(points[index], {});
                float values[2] = {point.x, point.y};
                ImGui::SetNextItemWidth(-28.0f);
                if (ImGui::DragFloat2("##point", values, 1.0f, 0.0f, 0.0f, "%.1f"))
                {
                    points[index] = writeVec2({values[0], values[1]});
                    modified = true;
                }
                trackActivity(aEdit);
                ImGui::SameLine();
                if (ImGui::SmallButton("x"))
                {
                    removeIndex = static_cast<int>(index);
                }
                ImGui::PopID();
            }
            if (removeIndex >= 0 && points.size() > 2)
            {
                points.erase(points.begin() + removeIndex);
                modified = true;
                aEdit.finished = true;
            }
            if (ImGui::SmallButton("+ point"))
            {
                Vec2 last = points.empty() ? Vec2{} : readVec2(points.back(), {});
                points.push_back(writeVec2(last + Vec2{100.0f, 0.0f}));
                modified = true;
                aEdit.finished = true;
            }
            if (modified)
            {
                aPoints = Value(std::move(points));
            }
            return modified;
        }
    }

    Vec2 imguiToScene(const ImVec2& aPoint)
    {
        const FrameMapping mapping = frameMapping();
        const float frameX = aPoint.x * mapping.frame.x / mapping.display.x;
        const float frameY = mapping.frame.y - aPoint.y * mapping.frame.y / mapping.display.y;
        return {(frameX - mapping.viewport.origin.x) / mapping.scaleX, (frameY - mapping.viewport.origin.y) / mapping.scaleY};
    }

    ImVec2 sceneToImGui(const Vec2& aPoint)
    {
        const FrameMapping mapping = frameMapping();
        const float frameX = aPoint.x * mapping.scaleX + mapping.viewport.origin.x;
        const float frameY = aPoint.y * mapping.scaleY + mapping.viewport.origin.y;
        return ImVec2(frameX * mapping.display.x / mapping.frame.x, (mapping.frame.y - frameY) * mapping.display.y / mapping.frame.y);
    }

    float sceneToImGuiScale()
    {
        const FrameMapping mapping = frameMapping();
        return mapping.scaleX * mapping.display.x / mapping.frame.x;
    }

    PropertyEdit editProperty(const PropertySchema& aProperty, const Value& aValue, PropertyEditorContext& aContext)
    {
        PropertyEdit edit;
        ImGui::PushID(aProperty.id.c_str());
        beginRow(aProperty, aContext);

        switch (aProperty.type)
        {
        case PropertyType::Bool:
        {
            bool value = aValue.asBool(aProperty.defaultValue.asBool());
            if (ImGui::Checkbox("##value", &value))
            {
                edit.value = Value(value);
                edit.finished = true;
            }
            break;
        }
        case PropertyType::Int:
        {
            int value = static_cast<int>(aValue.asInt(aProperty.defaultValue.asInt()));
            const int minValue = static_cast<int>(aProperty.minValue);
            const int maxValue = static_cast<int>(aProperty.maxValue);
            if (ImGui::DragInt("##value", &value, 1.0f, aProperty.hasRange() ? minValue : 0, aProperty.hasRange() ? maxValue : 0))
            {
                edit.value = Value(value);
            }
            trackActivity(edit);
            break;
        }
        case PropertyType::Float:
        {
            float value = aValue.asFloat(aProperty.defaultValue.asFloat());
            const float speed = aProperty.step > 0.0f ? aProperty.step : 1.0f;
            if (ImGui::DragFloat("##value", &value, speed, aProperty.hasRange() ? aProperty.minValue : 0.0f, aProperty.hasRange() ? aProperty.maxValue : 0.0f, "%.3f"))
            {
                edit.value = Value(value);
            }
            trackActivity(edit);
            break;
        }
        case PropertyType::Vec2:
        {
            const Vec2 vector = readVec2(aValue, readVec2(aProperty.defaultValue, {}));
            float values[2] = {vector.x, vector.y};
            const float speed = aProperty.step > 0.0f ? aProperty.step : 1.0f;
            if (ImGui::DragFloat2("##value", values, speed, aProperty.hasRange() ? aProperty.minValue : 0.0f, aProperty.hasRange() ? aProperty.maxValue : 0.0f, "%.2f"))
            {
                edit.value = writeVec2({values[0], values[1]});
            }
            trackActivity(edit);
            break;
        }
        case PropertyType::Color:
        {
            const Color color = readColor(aValue, readColor(aProperty.defaultValue, {}));
            float values[3] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f};
            if (ImGui::ColorEdit3("##value", values))
            {
                edit.value = writeColor({static_cast<uint8_t>(std::lround(values[0] * 255.0f)), static_cast<uint8_t>(std::lround(values[1] * 255.0f)), static_cast<uint8_t>(std::lround(values[2] * 255.0f)), 255});
            }
            trackActivity(edit);
            break;
        }
        case PropertyType::String:
        case PropertyType::Text:
        {
            std::string text = aValue.asString(aProperty.defaultValue.asString());
            const bool changed = aProperty.type == PropertyType::Text ? ImGui::InputTextMultiline("##value", &text, ImVec2(-28.0f, ImGui::GetTextLineHeight() * 3.0f)) : ImGui::InputText("##value", &text);
            if (changed)
            {
                edit.value = Value(text);
            }
            trackActivity(edit);
            break;
        }
        case PropertyType::Asset:
        {
            std::string text = aValue.asString(aProperty.defaultValue.asString());
            ImGui::SetNextItemWidth(aContext.allowReset ? -60.0f : -32.0f);
            if (ImGui::InputText("##value", &text, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                edit.value = Value(text);
                edit.finished = true;
            }
            acceptDropPayload("HG_ASSET", text, edit);
            if (aContext.app)
            {
                const bool prefab = std::find(aProperty.options.begin(), aProperty.options.end(), ".json") != aProperty.options.end();
                stringPicker("##assets", prefab ? aContext.app->prefabFiles() : aContext.app->assetFiles(), aProperty.options, text, edit);
            }
            break;
        }
        case PropertyType::SpriteFrame:
        {
            std::string text = aValue.asString(aProperty.defaultValue.asString());
            ImGui::SetNextItemWidth(aContext.allowReset ? -60.0f : -32.0f);
            if (ImGui::InputText("##value", &text, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                edit.value = Value(text);
                edit.finished = true;
            }
            acceptDropPayload("HG_FRAME", text, edit);
            if (aContext.app)
            {
                stringPicker("##frames", aContext.app->atlasFrames(), {}, text, edit);
            }
            break;
        }
        case PropertyType::Enum:
        {
            const std::string current = aValue.asString(aProperty.defaultValue.asString());
            if (ImGui::BeginCombo("##value", current.c_str()))
            {
                for (const auto& option : aProperty.options)
                {
                    if (ImGui::Selectable(option.c_str(), option == current))
                    {
                        edit.value = Value(option);
                        edit.finished = true;
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case PropertyType::ObjectRef:
        {
            const ObjectId current = static_cast<ObjectId>(std::max<int64_t>(0, aValue.asInt()));
            std::vector<const ObjectDesc*> objects;
            if (aContext.app && aContext.app->document())
            {
                collectObjects(aContext.app->document()->objects, objects);
            }
            std::string preview = "None";
            for (const ObjectDesc* object : objects)
            {
                if (object->uid == current)
                {
                    preview = objectLabel(object->name, object->uid);
                }
            }
            if (ImGui::BeginCombo("##value", preview.c_str()))
            {
                if (ImGui::Selectable("None", current == kInvalidObjectId))
                {
                    edit.value = Value(0);
                    edit.finished = true;
                }
                for (const ObjectDesc* object : objects)
                {
                    if (!aProperty.refType.empty() && aContext.app && !aContext.app->schema().isKindOf(aContext.app->objectType(object->uid), aProperty.refType))
                    {
                        continue;
                    }
                    if (ImGui::Selectable(objectLabel(object->name, object->uid).c_str(), object->uid == current))
                    {
                        edit.value = Value(object->uid);
                        edit.finished = true;
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case PropertyType::SceneRef:
        case PropertyType::TimelineRef:
        {
            const std::string current = aValue.asString();
            std::vector<std::string> options;
            if (aContext.app)
            {
                if (aProperty.type == PropertyType::SceneRef)
                {
                    options = aContext.app->sceneIds();
                }
                else if (const SceneDocument* document = aContext.app->document())
                {
                    for (const auto& timeline : document->timelines)
                    {
                        options.push_back(timeline.id);
                    }
                }
            }
            if (ImGui::BeginCombo("##value", current.c_str()))
            {
                for (const auto& option : options)
                {
                    if (ImGui::Selectable(option.c_str(), option == current))
                    {
                        edit.value = Value(option);
                        edit.finished = true;
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case PropertyType::ActionList:
        {
            ImGui::NewLine();
            Value actions = aValue.isArray() ? aValue : aProperty.defaultValue;
            bool active = false;
            ImGui::Indent(12.0f);
            if (editActionList("##actions", actions, aContext, active))
            {
                edit.value = actions;
            }
            ImGui::Unindent(12.0f);
            edit.active = active;
            edit.finished = edit.value.has_value() && !active;
            break;
        }
        case PropertyType::PointList:
        {
            ImGui::NewLine();
            Value points = aValue.isArray() ? aValue : aProperty.defaultValue;
            ImGui::Indent(12.0f);
            if (editPointList(points, edit))
            {
                edit.value = points;
            }
            ImGui::Unindent(12.0f);
            break;
        }
        case PropertyType::Any:
        {
            PropertyEdit json = editValueJson("##value", aValue);
            edit.value = json.value;
            edit.finished = json.finished;
            break;
        }
        }

        if (aContext.allowReset && aContext.overridden && aProperty.type != PropertyType::ActionList && aProperty.type != PropertyType::PointList)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("x"))
            {
                edit.reset = true;
                edit.finished = true;
            }
            ImGui::SetItemTooltip("Reset to default");
        }
        ImGui::PopID();
        return edit;
    }

    PropertyEdit editValueJson(const char* aLabel, const Value& aValue)
    {
        PropertyEdit edit;
        ImGuiStorage* storage = ImGui::GetStateStorage();
        const ImGuiID errorId = ImGui::GetID("json_error");
        std::string text = writeJsonCompact(aValue);
        if (ImGui::InputText(aLabel, &text, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            auto parsed = parseJson(text);
            storage->SetBool(errorId, !parsed.ok());
            if (parsed)
            {
                edit.value = parsed.take();
                edit.finished = true;
            }
        }
        if (storage->GetBool(errorId, false))
        {
            ImGui::TextColored(EditorColors::kErrorText, "invalid JSON");
        }
        return edit;
    }

    bool editActionList(const char* aLabel, Value& aActions, PropertyEditorContext& aContext, bool& aActive)
    {
        ImGui::PushID(aLabel);
        ValueArray actions = aActions.asArray();
        bool modified = false;
        int removeIndex = -1;
        int moveUpIndex = -1;
        const SchemaRegistry& schema = aContext.app ? aContext.app->schema() : builtinSchemas();

        for (size_t index = 0; index < actions.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));
            ValueObject action = actions[index].asObject();
            const std::string name = action.get("do").asString();

            ImGui::SetNextItemWidth(170.0f);
            if (ImGui::BeginCombo("##do", name.c_str()))
            {
                for (const auto& candidate : schema.actions())
                {
                    if (ImGui::Selectable(candidate.id.c_str(), candidate.id == name))
                    {
                        ValueObject replacement{{"do", Value(candidate.id)}};
                        for (const auto& argument : candidate.arguments)
                        {
                            replacement.set(argument.id, argument.defaultValue);
                        }
                        action = std::move(replacement);
                        modified = true;
                    }
                    if (!candidate.description.empty())
                    {
                        ImGui::SetItemTooltip("%s", candidate.description.c_str());
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (index > 0 && ImGui::ArrowButton("##up", ImGuiDir_Up))
            {
                moveUpIndex = static_cast<int>(index);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("x"))
            {
                removeIndex = static_cast<int>(index);
            }

            if (const ActionSchema* actionSchema = schema.findAction(action.get("do").asString()))
            {
                ImGui::Indent(12.0f);
                for (const auto& argument : actionSchema->arguments)
                {
                    PropertyEditorContext argumentContext;
                    argumentContext.app = aContext.app;
                    PropertyEdit edit = editProperty(argument, propertyValueOrDefault(action, argument), argumentContext);
                    aActive = aActive || edit.active;
                    if (edit.value)
                    {
                        action.set(argument.id, *edit.value);
                        modified = true;
                    }
                }
                ImGui::Unindent(12.0f);
            }
            actions[index] = Value(std::move(action));
            ImGui::Separator();
            ImGui::PopID();
        }

        if (removeIndex >= 0)
        {
            actions.erase(actions.begin() + removeIndex);
            modified = true;
        }
        if (moveUpIndex > 0)
        {
            std::swap(actions[static_cast<size_t>(moveUpIndex)], actions[static_cast<size_t>(moveUpIndex - 1)]);
            modified = true;
        }
        if (ImGui::SmallButton("+ action"))
        {
            actions.push_back(Value(ValueObject{{"do", Value("play_sound")}, {"file", Value("")}}));
            modified = true;
        }
        if (modified)
        {
            aActions = Value(std::move(actions));
        }
        ImGui::PopID();
        return modified;
    }

    void helpMarker(const std::string& aText)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        ImGui::SetItemTooltip("%s", aText.c_str());
    }

    std::string objectLabel(const std::string& aName, uint64_t aUid)
    {
        return (aName.empty() ? std::string("<unnamed>") : aName) + "  #" + std::to_string(aUid);
    }
}
