#include "Editor/Panels/Panels.h"

#include "Core/Edit/SceneCommands.h"
#include "Editor/EditorApp.h"
#include "Editor/EditorGui.h"

#include "ImGui/misc/cpp/imgui_stdlib.h"

namespace hg
{
    namespace
    {
        void finishEdit(EditorApp& aApp, const PropertyEdit& aEdit)
        {
            if (aEdit.finished || (!aEdit.active && aEdit.changed()))
            {
                aApp.endGesture();
            }
        }

        void drawSceneSettings(EditorApp& aApp, const SceneDocument& aDocument)
        {
            ImGui::Text("Scene '%s'", aDocument.id.c_str());
            ImGui::TextDisabled("%s, %zu objects, %zu timelines", std::string(sceneKindName(aDocument.kind)).c_str(), aDocument.objects.size(), aDocument.timelines.size());
            ImGui::Separator();
            for (const auto& setting : aApp.schema().sceneSettings())
            {
                PropertyEditorContext context;
                context.app = &aApp;
                context.overridden = aDocument.settings.contains(setting.id);
                PropertyEdit edit = editProperty(setting, propertyValueOrDefault(aDocument.settings, setting), context);
                if (edit.value)
                {
                    aApp.execute(std::make_unique<SetSettingCommand>(setting.id, *edit.value), edit.active);
                }
                finishEdit(aApp, edit);
            }
        }

        void drawKeyButton(EditorApp& aApp, ObjectId aUid, const PropertySchema& aProperty, const Value& aValue)
        {
            if (!aProperty.animatable || !aApp.selectedTimeline())
            {
                return;
            }
            ImGui::SameLine(ImGui::GetWindowWidth() - 34.0f);
            ImGui::PushID(aProperty.id.c_str());
            if (ImGui::SmallButton("K"))
            {
                aApp.recordKeys({{aUid, aProperty.id, aValue}}, false);
            }
            ImGui::SetItemTooltip("Add key at the playhead of timeline '%s'", aApp.timelineState().timelineId.c_str());
            ImGui::PopID();
        }

        void drawComponents(EditorApp& aApp, const ObjectDesc& aObject)
        {
            ImGui::SeparatorText("Components");
            int removeIndex = -1;
            for (size_t index = 0; index < aObject.components.size(); ++index)
            {
                const ComponentDesc& component = aObject.components[index];
                ImGui::PushID(static_cast<int>(index));
                const bool open = ImGui::CollapsingHeader(component.type.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
                ImGui::SameLine(ImGui::GetWindowWidth() - 36.0f);
                if (ImGui::SmallButton("x"))
                {
                    removeIndex = static_cast<int>(index);
                }
                ImGui::SetItemTooltip("Remove component");
                if (open)
                {
                    const ComponentSchema* schema = aApp.schema().findComponent(component.type);
                    if (!schema)
                    {
                        ImGui::TextColored(EditorColors::kErrorText, "Unknown component type");
                    }
                    else
                    {
                        if (!schema->description.empty())
                        {
                            ImGui::TextDisabled("%s", schema->description.c_str());
                        }
                        for (const auto& property : schema->properties)
                        {
                            PropertyEditorContext context;
                            context.app = &aApp;
                            context.overridden = component.props.contains(property.id);
                            context.allowReset = true;
                            PropertyEdit edit = editProperty(property, propertyValueOrDefault(component.props, property), context);
                            if (edit.reset)
                            {
                                aApp.execute(std::make_unique<SetComponentPropertyCommand>(aObject.uid, index, property.id, std::nullopt));
                            }
                            else if (edit.value)
                            {
                                aApp.execute(std::make_unique<SetComponentPropertyCommand>(aObject.uid, index, property.id, *edit.value), edit.active);
                            }
                            finishEdit(aApp, edit);
                        }
                    }
                }
                ImGui::PopID();
            }
            if (removeIndex >= 0)
            {
                aApp.execute(std::make_unique<RemoveComponentCommand>(aObject.uid, static_cast<size_t>(removeIndex)));
            }

            if (ImGui::BeginCombo("##add_component", "Add component..."))
            {
                for (const auto& component : aApp.schema().components())
                {
                    if (aObject.findComponent(component.id))
                    {
                        continue;
                    }
                    if (ImGui::Selectable(component.id.c_str()))
                    {
                        aApp.execute(std::make_unique<AddComponentCommand>(aObject.uid, ComponentDesc{component.id, {}}));
                    }
                    if (!component.description.empty())
                    {
                        ImGui::SetItemTooltip("%s", component.description.c_str());
                    }
                }
                ImGui::EndCombo();
            }
        }

        void drawUnknownProperties(EditorApp& aApp, const ObjectDesc& aObject, const std::string& aType)
        {
            bool header = false;
            for (const auto& member : aObject.props)
            {
                if (aApp.schema().findProperty(aType, member.key))
                {
                    continue;
                }
                if (!header)
                {
                    ImGui::SeparatorText("Unknown properties");
                    header = true;
                }
                ImGui::PushID(member.key.c_str());
                ImGui::TextColored(EditorColors::kWarningText, "%s", member.key.c_str());
                ImGui::SameLine(130.0f);
                ImGui::SetNextItemWidth(-28.0f);
                PropertyEdit edit = editValueJson("##json", member.value);
                if (edit.value)
                {
                    aApp.setObjectProperty(aObject.uid, member.key, *edit.value, false);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("x"))
                {
                    aApp.setObjectProperty(aObject.uid, member.key, std::nullopt, false);
                }
                ImGui::PopID();
            }
        }

        void drawObject(EditorApp& aApp, const ObjectDesc& aObject)
        {
            static ObjectId editedUid = kInvalidObjectId;
            static std::string nameBuffer;
            if (editedUid != aObject.uid || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                if (!ImGui::IsAnyItemActive())
                {
                    editedUid = aObject.uid;
                    nameBuffer = aObject.name;
                }
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText("##name", &nameBuffer);
            if (ImGui::IsItemDeactivatedAfterEdit() && nameBuffer != aObject.name)
            {
                aApp.renameObject(aObject.uid, nameBuffer);
            }

            const std::string type = aApp.objectType(aObject.uid);
            ImGui::TextDisabled("%s  #%llu", type.c_str(), static_cast<unsigned long long>(aObject.uid));
            if (!aObject.prefab.empty())
            {
                ImGui::TextColored(EditorColors::kOverrideText, "Prefab: %s", aObject.prefab.c_str());
                ImGui::TextDisabled("Blue labels are overridden in this instance");
            }
            if (const TypeSchema* typeSchema = aApp.schema().findType(type); typeSchema && !typeSchema->description.empty())
            {
                ImGui::TextDisabled("%s", typeSchema->description.c_str());
            }
            if (type == "Path")
            {
                ImGui::TextDisabled("Viewport: drag points, Ctrl+click adds, Alt+click removes");
            }
            if (aApp.timelineState().recording)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "REC: animatable edits become keys");
            }

            ImGui::SeparatorText("Properties");
            const ValueObject* prefabProps = aApp.prefabProps(aObject.uid);
            for (const PropertySchema* property : aApp.schema().collectProperties(type))
            {
                if (property->hidden)
                {
                    continue;
                }
                const Value* explicitValue = aObject.props.find(property->id);
                const Value* inherited = prefabProps ? prefabProps->find(property->id) : nullptr;
                const Value& value = explicitValue ? *explicitValue : (inherited ? *inherited : property->defaultValue);

                PropertyEditorContext context;
                context.app = &aApp;
                context.overridden = explicitValue != nullptr;
                context.allowReset = true;
                PropertyEdit edit = editProperty(*property, value, context);
                if (edit.reset)
                {
                    aApp.setObjectProperty(aObject.uid, property->id, std::nullopt, false);
                }
                else if (edit.value)
                {
                    aApp.setObjectProperty(aObject.uid, property->id, *edit.value, edit.active);
                }
                finishEdit(aApp, edit);
                drawKeyButton(aApp, aObject.uid, *property, value);
            }

            drawUnknownProperties(aApp, aObject, type);
            drawComponents(aApp, aObject);
        }

        void drawMultiSelection(EditorApp& aApp, const SceneDocument& aDocument)
        {
            ImGui::Text("%zu objects selected", aApp.selection().size());
            ImGui::TextDisabled("Shared transform properties apply to every selected object");
            ImGui::SeparatorText("Shared");
            const ObjectDesc* primary = aDocument.findObject(aApp.primarySelection());
            if (!primary)
            {
                return;
            }
            for (const PropertySchema* property : aApp.schema().collectProperties("Node"))
            {
                const Value* current = aApp.effectiveProperty(primary->uid, property->id);
                PropertyEditorContext context;
                context.app = &aApp;
                PropertyEdit edit = editProperty(*property, current ? *current : property->defaultValue, context);
                if (edit.value)
                {
                    std::vector<std::tuple<ObjectId, std::string, Value>> changes;
                    for (ObjectId uid : aApp.selection())
                    {
                        changes.emplace_back(uid, property->id, *edit.value);
                    }
                    aApp.setObjectProperties(changes, "Change " + property->id, edit.active);
                }
                finishEdit(aApp, edit);
            }
        }
    }

    void drawInspectorPanel(EditorApp& aApp)
    {
        if (!ImGui::Begin(PanelNames::kInspector))
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

        if (aApp.selection().empty())
        {
            const SceneDocument snapshot = *document;
            drawSceneSettings(aApp, snapshot);
        }
        else if (aApp.selection().size() > 1)
        {
            const SceneDocument snapshot = *document;
            drawMultiSelection(aApp, snapshot);
        }
        else if (const ObjectDesc* object = document->findObject(aApp.primarySelection()))
        {
            const ObjectDesc snapshot = *object;
            drawObject(aApp, snapshot);
        }
        ImGui::End();
    }
}
