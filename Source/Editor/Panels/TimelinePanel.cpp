#include "Editor/Panels/Panels.h"

#include "Core/Animation/TimelineEvaluator.h"
#include "Core/Base/ValueConvert.h"
#include "Editor/EditorApp.h"
#include "Editor/EditorGui.h"

#include "ImGui/misc/cpp/imgui_stdlib.h"

#include <cmath>
#include <cstdio>

namespace hg
{
    namespace
    {
        constexpr float kRowHeight = 24.0f;
        constexpr float kLabelColumnWidth = 260.0f;
        constexpr float kKeyRadius = 6.0f;
        constexpr float kTimeSnap = 0.05f;
        constexpr float kInspectorHeight = 150.0f;

        struct LaneGeometry
        {
            float x0 = 0.0f;
            float width = 1.0f;
            float duration = 1.0f;

            float timeToX(float aTime) const
            {
                return x0 + aTime / std::max(0.001f, duration) * width;
            }

            float xToTime(float aX) const
            {
                return std::clamp((aX - x0) / std::max(1.0f, width) * duration, 0.0f, duration);
            }
        };

        float snapTime(float aTime)
        {
            if (ImGui::GetIO().KeyShift)
            {
                return aTime;
            }
            return std::round(aTime / kTimeSnap) * kTimeSnap;
        }

        std::string objectName(const SceneDocument& aDocument, ObjectId aUid)
        {
            const ObjectDesc* object = aDocument.findObject(aUid);
            return object ? object->name : "#" + std::to_string(aUid);
        }

        std::string trackLabel(const SceneDocument& aDocument, const TrackDesc& aTrack)
        {
            const std::string target = aTrack.target.isCamera() ? std::string("Camera") : objectName(aDocument, aTrack.target.uid);
            switch (aTrack.kind)
            {
            case TrackKind::Property:
                return target + "." + aTrack.property;
            case TrackKind::Path:
                return target + " along " + objectName(aDocument, aTrack.path);
            case TrackKind::Event:
                return "Events";
            }
            return target;
        }

        std::string uniqueTimelineId(const SceneDocument& aDocument, const std::string& aBase)
        {
            if (!aDocument.findTimeline(aBase))
            {
                return aBase;
            }
            for (int index = 1; index < 10000; ++index)
            {
                const std::string candidate = aBase + "_" + std::to_string(index);
                if (!aDocument.findTimeline(candidate))
                {
                    return candidate;
                }
            }
            return aBase;
        }

        TimelineDesc* findTimeline(std::vector<TimelineDesc>& aTimelines, const std::string& aId)
        {
            for (auto& timeline : aTimelines)
            {
                if (timeline.id == aId)
                {
                    return &timeline;
                }
            }
            return nullptr;
        }

        const PropertySchema* trackProperty(EditorApp& aApp, const TrackDesc& aTrack)
        {
            if (aTrack.target.isCamera())
            {
                return aApp.schema().findCameraProperty(aTrack.property);
            }
            return aApp.schema().findProperty(aApp.objectType(aTrack.target.uid), aTrack.property);
        }

        Value currentTrackValue(EditorApp& aApp, const TrackDesc& aTrack, float aTime)
        {
            if (auto sampled = sampleKeyframes(aTrack.keys, aTime))
            {
                return *sampled;
            }
            if (aTrack.target.isCamera())
            {
                const CameraState camera = aApp.gameCameraPreview();
                if (aTrack.property == "center")
                {
                    return writeVec2(camera.center);
                }
                if (aTrack.property == "zoom")
                {
                    return Value(camera.zoom);
                }
                const PropertySchema* property = aApp.schema().findCameraProperty(aTrack.property);
                return property ? property->defaultValue : Value();
            }
            if (const Value* value = aApp.effectiveProperty(aTrack.target.uid, aTrack.property))
            {
                return *value;
            }
            const PropertySchema* property = trackProperty(aApp, aTrack);
            return property ? property->defaultValue : Value();
        }

        void commit(EditorApp& aApp, std::vector<TimelineDesc> aTimelines, const std::string& aLabel, const std::string& aMergeKey = {})
        {
            aApp.setTimelines(std::move(aTimelines), aLabel, aMergeKey);
        }

        void collectPaths(const std::vector<ObjectDesc>& aObjects, std::vector<const ObjectDesc*>& aOut)
        {
            for (const auto& object : aObjects)
            {
                if (object.type == "Path")
                {
                    aOut.push_back(&object);
                }
                collectPaths(object.children, aOut);
            }
        }

        void drawAddTrackMenu(EditorApp& aApp, std::vector<TimelineDesc> aTimelines, TimelineDesc& aTimeline)
        {
            const SceneDocument& document = *aApp.document();
            TimelineState& state = aApp.timelineState();
            const float time = state.playhead;
            const ObjectId selected = aApp.primarySelection();
            bool added = false;

            if (ImGui::BeginMenu("Selected object property", selected != kInvalidObjectId))
            {
                for (const PropertySchema* property : aApp.schema().collectProperties(aApp.objectType(selected)))
                {
                    if (!property->animatable || !ImGui::MenuItem(property->id.c_str()))
                    {
                        continue;
                    }
                    TrackDesc track;
                    track.target = TargetRef::object(selected);
                    track.property = property->id;
                    const Value* value = aApp.effectiveProperty(selected, property->id);
                    track.keys.push_back({time, value ? *value : property->defaultValue, {}});
                    aTimeline.tracks.push_back(std::move(track));
                    added = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Camera"))
            {
                for (const auto& property : aApp.schema().cameraProperties())
                {
                    if (!ImGui::MenuItem(property.id.c_str()))
                    {
                        continue;
                    }
                    TrackDesc track;
                    track.target = TargetRef::camera();
                    track.property = property.id;
                    track.keys.push_back({time, currentTrackValue(aApp, track, time), {}});
                    aTimeline.tracks.push_back(std::move(track));
                    added = true;
                }
                ImGui::EndMenu();
            }

            std::vector<const ObjectDesc*> paths;
            collectPaths(document.objects, paths);
            auto pathMenu = [&](const char* aLabel, TargetRef aTarget, bool aEnabled)
            {
                if (!ImGui::BeginMenu(aLabel, aEnabled && !paths.empty()))
                {
                    return;
                }
                for (const ObjectDesc* path : paths)
                {
                    if (!ImGui::MenuItem(objectLabel(path->name, path->uid).c_str()))
                    {
                        continue;
                    }
                    TrackDesc track;
                    track.kind = TrackKind::Path;
                    track.target = aTarget;
                    track.path = path->uid;
                    track.start = time;
                    track.duration = std::max(0.5f, std::min(2.0f, aTimeline.duration - time));
                    track.ease = Easing::of(EaseType::SineInOut);
                    aTimeline.duration = std::max(aTimeline.duration, track.start + track.duration);
                    aTimeline.tracks.push_back(std::move(track));
                    added = true;
                }
                ImGui::EndMenu();
            };
            pathMenu("Selected object along path", TargetRef::object(selected), selected != kInvalidObjectId);
            pathMenu("Camera along path", TargetRef::camera(), true);

            if (ImGui::MenuItem("Events"))
            {
                TrackDesc track;
                track.kind = TrackKind::Event;
                track.events.push_back({time, ValueObject{{"do", Value("play_sound")}, {"file", Value("")}}});
                aTimeline.tracks.push_back(std::move(track));
                added = true;
            }

            if (added)
            {
                state.selectedTrack = static_cast<int>(aTimeline.tracks.size()) - 1;
                state.selectedItem = 0;
                commit(aApp, std::move(aTimelines), "Add track");
            }
        }

        void drawHeader(EditorApp& aApp, const SceneDocument& aDocument)
        {
            TimelineState& state = aApp.timelineState();
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::BeginCombo("##timeline", state.timelineId.empty() ? "<no timeline>" : state.timelineId.c_str()))
            {
                for (const auto& timeline : aDocument.timelines)
                {
                    if (ImGui::Selectable(timeline.id.c_str(), timeline.id == state.timelineId))
                    {
                        aApp.stopPreview();
                        state.timelineId = timeline.id;
                        state.playhead = 0.0f;
                        state.selectedTrack = -1;
                        state.selectedItem = -1;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("New"))
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                TimelineDesc timeline;
                timeline.id = uniqueTimelineId(aDocument, "timeline");
                timeline.duration = 3.0f;
                timelines.push_back(timeline);
                aApp.stopPreview();
                state.timelineId = timeline.id;
                state.playhead = 0.0f;
                commit(aApp, std::move(timelines), "New timeline");
            }
            const TimelineDesc* current = aDocument.findTimeline(state.timelineId);
            ImGui::BeginDisabled(!current);
            ImGui::SameLine();
            if (ImGui::Button("Rename"))
            {
                ImGui::OpenPopup("Rename timeline");
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate") && current)
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                TimelineDesc copy = *current;
                copy.id = uniqueTimelineId(aDocument, current->id + "_copy");
                timelines.push_back(copy);
                state.timelineId = copy.id;
                commit(aApp, std::move(timelines), "Duplicate timeline");
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete") && current)
            {
                std::vector<TimelineDesc> timelines;
                for (const auto& timeline : aDocument.timelines)
                {
                    if (timeline.id != current->id)
                    {
                        timelines.push_back(timeline);
                    }
                }
                aApp.stopPreview();
                commit(aApp, std::move(timelines), "Delete timeline");
            }
            ImGui::EndDisabled();

            if (ImGui::BeginPopup("Rename timeline"))
            {
                static std::string buffer;
                if (ImGui::IsWindowAppearing())
                {
                    buffer = state.timelineId;
                    ImGui::SetKeyboardFocusHere();
                }
                if (ImGui::InputText("Id", &buffer, ImGuiInputTextFlags_EnterReturnsTrue) && !buffer.empty() && !aDocument.findTimeline(buffer))
                {
                    std::vector<TimelineDesc> timelines = aDocument.timelines;
                    if (TimelineDesc* timeline = findTimeline(timelines, state.timelineId))
                    {
                        timeline->id = buffer;
                        state.timelineId = buffer;
                        commit(aApp, std::move(timelines), "Rename timeline");
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::TextDisabled("Actions that play this timeline are not renamed");
                ImGui::EndPopup();
            }
        }

        void drawTransport(EditorApp& aApp, const SceneDocument& aDocument, const TimelineDesc& aTimeline)
        {
            TimelineState& state = aApp.timelineState();
            if (ImGui::Button("|<"))
            {
                aApp.seekPreview(0.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button(state.playing ? "Pause" : "Play"))
            {
                if (state.playing)
                {
                    aApp.pausePreview();
                }
                else
                {
                    aApp.playPreview();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                aApp.stopPreview();
            }
            ImGui::SetItemTooltip("Stop preview and restore the scene");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70.0f);
            const float speeds[] = {0.25f, 0.5f, 1.0f, 2.0f};
            if (ImGui::BeginCombo("##speed", (std::to_string(state.speed).substr(0, 4) + "x").c_str()))
            {
                for (float speed : speeds)
                {
                    if (ImGui::Selectable((std::to_string(speed).substr(0, 4) + "x").c_str(), speed == state.speed))
                    {
                        state.speed = speed;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Text("%.2f / %.2f s", state.playhead, aTimeline.duration);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, state.recording ? ImVec4(0.75f, 0.15f, 0.15f, 1.0f) : ImGui::GetStyleColorVec4(ImGuiCol_Button));
            if (ImGui::Button("Rec"))
            {
                state.recording = !state.recording;
                if (state.recording && !state.previewActive)
                {
                    aApp.startPreview();
                }
            }
            ImGui::PopStyleColor();
            ImGui::SetItemTooltip("Record: moving objects or editing animatable properties writes keys at the playhead");
            ImGui::SameLine();
            ImGui::Checkbox("Camera view", &state.viewThroughCamera);
            ImGui::SetItemTooltip("Look through the animated game camera while previewing");

            ImGui::SameLine();
            float duration = aTimeline.duration;
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::DragFloat("Duration", &duration, 0.05f, 0.1f, 600.0f, "%.2f s"))
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                findTimeline(timelines, aTimeline.id)->duration = std::max(0.1f, duration);
                commit(aApp, std::move(timelines), "Change duration", "duration");
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                aApp.endGesture();
            }
            ImGui::SameLine();
            bool loop = aTimeline.loop;
            if (ImGui::Checkbox("Loop", &loop))
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                findTimeline(timelines, aTimeline.id)->loop = loop;
                commit(aApp, std::move(timelines), "Toggle loop");
            }
            ImGui::SameLine();
            if (ImGui::Button("Add track"))
            {
                ImGui::OpenPopup("AddTrack");
            }
            if (ImGui::BeginPopup("AddTrack"))
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                if (TimelineDesc* timeline = findTimeline(timelines, aTimeline.id))
                {
                    drawAddTrackMenu(aApp, timelines, *timeline);
                }
                ImGui::EndPopup();
            }
        }

        void drawRuler(EditorApp& aApp, const LaneGeometry& aLane, float aDuration)
        {
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##ruler", ImVec2(aLane.width, kRowHeight));
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddRectFilled(origin, ImVec2(origin.x + aLane.width, origin.y + kRowHeight), IM_COL32(40, 40, 46, 255));
            float step = 0.1f;
            while (aLane.width / (aDuration / step) < 50.0f)
            {
                step *= step < 1.0f ? 2.5f : 2.0f;
            }
            for (float time = 0.0f; time <= aDuration + 1e-4f; time += step)
            {
                const float x = aLane.timeToX(time);
                draw->AddLine(ImVec2(x, origin.y + kRowHeight * 0.5f), ImVec2(x, origin.y + kRowHeight), IM_COL32(160, 160, 160, 255));
                char label[32];
                std::snprintf(label, sizeof(label), "%.2f", time);
                draw->AddText(ImVec2(x + 2.0f, origin.y + 1.0f), IM_COL32(180, 180, 180, 255), label);
            }
            if (ImGui::IsItemActive())
            {
                aApp.seekPreview(snapTime(aLane.xToTime(ImGui::GetIO().MousePos.x)));
            }
            const float playheadX = aLane.timeToX(aApp.timelineState().playhead);
            draw->AddTriangleFilled(ImVec2(playheadX - 6.0f, origin.y), ImVec2(playheadX + 6.0f, origin.y), ImVec2(playheadX, origin.y + 10.0f), EditorColors::kPlayhead);
            draw->AddLine(ImVec2(playheadX, origin.y), ImVec2(playheadX, origin.y + kRowHeight), EditorColors::kPlayhead, 2.0f);
        }

        void drawDiamond(ImDrawList* aDraw, const ImVec2& aCenter, ImU32 aColor)
        {
            aDraw->AddQuadFilled(ImVec2(aCenter.x, aCenter.y - kKeyRadius), ImVec2(aCenter.x + kKeyRadius, aCenter.y), ImVec2(aCenter.x, aCenter.y + kKeyRadius), ImVec2(aCenter.x - kKeyRadius, aCenter.y), aColor);
        }

        void drawLane(EditorApp& aApp, const SceneDocument& aDocument, const TimelineDesc& aTimeline, size_t aTrackIndex, const LaneGeometry& aLane)
        {
            TimelineState& state = aApp.timelineState();
            const TrackDesc& track = aTimeline.tracks[aTrackIndex];
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##lane", ImVec2(aLane.width, kRowHeight), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool hovered = ImGui::IsItemHovered();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const float centerY = origin.y + kRowHeight * 0.5f;
            const bool trackSelected = state.selectedTrack == static_cast<int>(aTrackIndex);
            draw->AddRectFilled(origin, ImVec2(origin.x + aLane.width, origin.y + kRowHeight), trackSelected ? IM_COL32(60, 55, 45, 255) : IM_COL32(32, 32, 36, 255));

            const ImVec2 mouse = ImGui::GetIO().MousePos;
            int hitItem = -1;
            bool hitClipEdge = false;

            if (track.kind == TrackKind::Property)
            {
                for (size_t index = 1; index < track.keys.size(); ++index)
                {
                    draw->AddLine(ImVec2(aLane.timeToX(track.keys[index - 1].time), centerY), ImVec2(aLane.timeToX(track.keys[index].time), centerY), IM_COL32(120, 120, 130, 255), 2.0f);
                }
                for (size_t index = 0; index < track.keys.size(); ++index)
                {
                    const ImVec2 center{aLane.timeToX(track.keys[index].time), centerY};
                    const bool selected = trackSelected && state.selectedItem == static_cast<int>(index);
                    drawDiamond(draw, center, selected ? EditorColors::kKeySelected : EditorColors::kKey);
                    if (hovered && std::abs(mouse.x - center.x) <= kKeyRadius + 2.0f)
                    {
                        hitItem = static_cast<int>(index);
                    }
                }
            }
            else if (track.kind == TrackKind::Path)
            {
                const float x0 = aLane.timeToX(track.start);
                const float x1 = aLane.timeToX(track.start + track.duration);
                draw->AddRectFilled(ImVec2(x0, origin.y + 3.0f), ImVec2(x1, origin.y + kRowHeight - 3.0f), trackSelected ? IM_COL32(60, 170, 210, 255) : IM_COL32(40, 120, 160, 255), 4.0f);
                draw->AddRectFilled(ImVec2(x1 - 4.0f, origin.y + 3.0f), ImVec2(x1, origin.y + kRowHeight - 3.0f), IM_COL32(220, 240, 255, 255));
                draw->AddText(ImVec2(x0 + 4.0f, origin.y + 4.0f), IM_COL32(255, 255, 255, 255), easeTypeName(track.ease.type).data());
                if (hovered && mouse.x >= x0 && mouse.x <= x1)
                {
                    hitItem = 0;
                    hitClipEdge = mouse.x >= x1 - 6.0f;
                }
            }
            else
            {
                for (size_t index = 0; index < track.events.size(); ++index)
                {
                    const float x = aLane.timeToX(track.events[index].time);
                    const bool selected = trackSelected && state.selectedItem == static_cast<int>(index);
                    draw->AddRectFilled(ImVec2(x - 4.0f, origin.y + 3.0f), ImVec2(x + 4.0f, origin.y + kRowHeight - 3.0f), selected ? EditorColors::kKeySelected : IM_COL32(200, 160, 255, 255));
                    if (hovered && std::abs(mouse.x - x) <= 6.0f)
                    {
                        hitItem = static_cast<int>(index);
                    }
                }
            }

            const float playheadX = aLane.timeToX(state.playhead);
            draw->AddLine(ImVec2(playheadX, origin.y), ImVec2(playheadX, origin.y + kRowHeight), EditorColors::kPlayhead, 1.5f);

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                state.selectedTrack = static_cast<int>(aTrackIndex);
                state.selectedItem = hitItem;
                if (hitItem >= 0)
                {
                    state.draggingItem = true;
                    state.draggingClipEdge = hitClipEdge;
                    state.dragGrabOffset = track.kind == TrackKind::Path ? aLane.xToTime(mouse.x) - track.start : 0.0f;
                }
                if (track.target.isObject())
                {
                    aApp.select(track.target.uid);
                }
            }
            if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hitItem < 0)
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                TrackDesc& editable = findTimeline(timelines, aTimeline.id)->tracks[aTrackIndex];
                const float time = snapTime(aLane.xToTime(mouse.x));
                if (editable.kind == TrackKind::Property)
                {
                    editable.keys.push_back({time, currentTrackValue(aApp, editable, time), {}});
                    sortKeyframes(editable.keys);
                    commit(aApp, std::move(timelines), "Add key");
                }
                else if (editable.kind == TrackKind::Event)
                {
                    editable.events.push_back({time, ValueObject{{"do", Value("play_sound")}, {"file", Value("")}}});
                    sortEvents(editable.events);
                    commit(aApp, std::move(timelines), "Add event");
                }
            }

            if (state.draggingItem && state.selectedTrack == static_cast<int>(aTrackIndex) && state.selectedItem >= 0)
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f))
                    {
                        std::vector<TimelineDesc> timelines = aDocument.timelines;
                        TimelineDesc* timeline = findTimeline(timelines, aTimeline.id);
                        TrackDesc& editable = timeline->tracks[aTrackIndex];
                        const float time = snapTime(aLane.xToTime(mouse.x));
                        const size_t item = static_cast<size_t>(state.selectedItem);
                        if (editable.kind == TrackKind::Property && item < editable.keys.size())
                        {
                            Keyframe moved = editable.keys[item];
                            moved.time = time;
                            editable.keys.erase(editable.keys.begin() + static_cast<std::ptrdiff_t>(item));
                            auto position = std::upper_bound(editable.keys.begin(), editable.keys.end(), time, [](float aTime, const Keyframe& aKey)
                            {
                                return aTime < aKey.time;
                            });
                            state.selectedItem = static_cast<int>(std::distance(editable.keys.begin(), position));
                            editable.keys.insert(position, moved);
                        }
                        else if (editable.kind == TrackKind::Event && item < editable.events.size())
                        {
                            TimelineEvent moved = editable.events[item];
                            moved.time = time;
                            editable.events.erase(editable.events.begin() + static_cast<std::ptrdiff_t>(item));
                            auto position = std::upper_bound(editable.events.begin(), editable.events.end(), time, [](float aTime, const TimelineEvent& aEvent)
                            {
                                return aTime < aEvent.time;
                            });
                            state.selectedItem = static_cast<int>(std::distance(editable.events.begin(), position));
                            editable.events.insert(position, moved);
                        }
                        else if (editable.kind == TrackKind::Path)
                        {
                            if (state.draggingClipEdge)
                            {
                                editable.duration = std::max(0.05f, time - editable.start);
                            }
                            else
                            {
                                editable.start = std::max(0.0f, snapTime(aLane.xToTime(mouse.x) - state.dragGrabOffset));
                            }
                        }
                        timeline->duration = std::max(timeline->duration, trackEndTime(editable));
                        commit(aApp, std::move(timelines), "Move key", "key-drag");
                    }
                }
                else
                {
                    state.draggingItem = false;
                    aApp.endGesture();
                }
            }

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hitItem >= 0)
            {
                state.selectedTrack = static_cast<int>(aTrackIndex);
                state.selectedItem = hitItem;
                ImGui::OpenPopup("##key_context");
            }
            if (ImGui::BeginPopup("##key_context"))
            {
                std::vector<TimelineDesc> timelines = aDocument.timelines;
                TrackDesc& editable = findTimeline(timelines, aTimeline.id)->tracks[aTrackIndex];
                const size_t item = static_cast<size_t>(std::max(0, state.selectedItem));
                if (editable.kind == TrackKind::Property && item < editable.keys.size())
                {
                    if (ImGui::BeginMenu("Ease"))
                    {
                        for (EaseType type : allEaseTypes())
                        {
                            if (ImGui::MenuItem(easeTypeName(type).data(), nullptr, editable.keys[item].ease.type == type))
                            {
                                editable.keys[item].ease.type = type;
                                commit(aApp, timelines, "Change ease");
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem("Delete key"))
                    {
                        editable.keys.erase(editable.keys.begin() + static_cast<std::ptrdiff_t>(item));
                        state.selectedItem = -1;
                        commit(aApp, timelines, "Delete key");
                    }
                }
                else if (editable.kind == TrackKind::Event && item < editable.events.size())
                {
                    if (ImGui::MenuItem("Delete event"))
                    {
                        editable.events.erase(editable.events.begin() + static_cast<std::ptrdiff_t>(item));
                        state.selectedItem = -1;
                        commit(aApp, timelines, "Delete event");
                    }
                }
                ImGui::EndPopup();
            }
        }

        void drawEaseEditor(Easing& aEase, bool& aChanged)
        {
            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::BeginCombo("Ease", easeTypeName(aEase.type).data()))
            {
                for (EaseType type : allEaseTypes())
                {
                    if (ImGui::Selectable(easeTypeName(type).data(), aEase.type == type))
                    {
                        aEase.type = type;
                        aChanged = true;
                    }
                }
                ImGui::EndCombo();
            }
            if (aEase.type == EaseType::Bezier)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(240.0f);
                aChanged = ImGui::DragFloat4("##bezier", aEase.bezier.data(), 0.01f, -1.0f, 2.0f, "%.2f") || aChanged;
            }

            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImGui::SameLine();
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            const ImVec2 size{60.0f, 40.0f};
            ImGui::Dummy(size);
            draw->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(90, 90, 90, 255));
            ImVec2 previous = ImVec2(origin.x, origin.y + size.y);
            for (int step = 1; step <= 24; ++step)
            {
                const float t = static_cast<float>(step) / 24.0f;
                const ImVec2 point{origin.x + t * size.x, origin.y + size.y - aEase.apply(t) * size.y};
                draw->AddLine(previous, point, IM_COL32(255, 190, 80, 255), 1.5f);
                previous = point;
            }
        }

        void drawItemInspector(EditorApp& aApp, const SceneDocument& aDocument, const TimelineDesc& aTimeline)
        {
            TimelineState& state = aApp.timelineState();
            if (state.selectedTrack < 0 || state.selectedTrack >= static_cast<int>(aTimeline.tracks.size()))
            {
                ImGui::TextDisabled("Double-click a lane to add a key, drag keys to retime, right-click for ease and delete.");
                ImGui::TextDisabled("Rec + move objects in the viewport records keys at the playhead.");
                return;
            }
            std::vector<TimelineDesc> timelines = aDocument.timelines;
            TrackDesc& track = findTimeline(timelines, aTimeline.id)->tracks[static_cast<size_t>(state.selectedTrack)];
            const size_t item = static_cast<size_t>(std::max(0, state.selectedItem));
            bool changed = false;
            bool active = false;
            ImGui::Text("%s", trackLabel(aDocument, track).c_str());

            if (track.kind == TrackKind::Property && state.selectedItem >= 0 && item < track.keys.size())
            {
                Keyframe& key = track.keys[item];
                ImGui::SetNextItemWidth(100.0f);
                if (ImGui::DragFloat("Time", &key.time, 0.01f, 0.0f, aTimeline.duration, "%.2f s"))
                {
                    changed = true;
                }
                active = active || ImGui::IsItemActive();
                ImGui::SameLine();
                drawEaseEditor(key.ease, changed);
                if (const PropertySchema* property = trackProperty(aApp, track))
                {
                    PropertyEditorContext context;
                    context.app = &aApp;
                    PropertyEdit edit = editProperty(*property, key.value, context);
                    if (edit.value)
                    {
                        key.value = *edit.value;
                        changed = true;
                    }
                    active = active || edit.active;
                }
                if (changed)
                {
                    sortKeyframes(track.keys);
                }
            }
            else if (track.kind == TrackKind::Path)
            {
                std::vector<const ObjectDesc*> paths;
                collectPaths(aDocument.objects, paths);
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::BeginCombo("Path", objectName(aDocument, track.path).c_str()))
                {
                    for (const ObjectDesc* path : paths)
                    {
                        if (ImGui::Selectable(objectLabel(path->name, path->uid).c_str(), path->uid == track.path))
                        {
                            track.path = path->uid;
                            changed = true;
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90.0f);
                changed = ImGui::DragFloat("Start", &track.start, 0.01f, 0.0f, aTimeline.duration, "%.2f s") || changed;
                active = active || ImGui::IsItemActive();
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90.0f);
                changed = ImGui::DragFloat("Length", &track.duration, 0.01f, 0.05f, 600.0f, "%.2f s") || changed;
                active = active || ImGui::IsItemActive();
                changed = ImGui::Checkbox("Orient to path", &track.orient) || changed;
                ImGui::SameLine();
                changed = ImGui::Checkbox("Reverse", &track.reverse) || changed;
                ImGui::SameLine();
                drawEaseEditor(track.ease, changed);
            }
            else if (track.kind == TrackKind::Event && state.selectedItem >= 0 && item < track.events.size())
            {
                TimelineEvent& event = track.events[item];
                ImGui::SetNextItemWidth(100.0f);
                if (ImGui::DragFloat("Time", &event.time, 0.01f, 0.0f, aTimeline.duration, "%.2f s"))
                {
                    changed = true;
                    sortEvents(track.events);
                }
                active = active || ImGui::IsItemActive();
                Value actions(ValueArray{Value(event.action)});
                PropertyEditorContext context;
                context.app = &aApp;
                if (editActionList("##event", actions, context, active) && !actions.asArray().empty())
                {
                    event.action = actions.asArray().front().asObject();
                    changed = true;
                }
            }

            static bool wasActive = false;
            if (changed)
            {
                commit(aApp, std::move(timelines), "Edit timeline item", active ? "item-edit" : std::string());
            }
            if ((wasActive && !active) || (!active && changed))
            {
                aApp.endGesture();
            }
            wasActive = active;
        }
    }

    void drawTimelinePanel(EditorApp& aApp)
    {
        if (!ImGui::Begin(PanelNames::kTimeline))
        {
            ImGui::End();
            return;
        }
        const SceneDocument* documentPointer = aApp.document();
        if (!documentPointer)
        {
            ImGui::End();
            return;
        }
        const SceneDocument document = *documentPointer;
        TimelineState& state = aApp.timelineState();

        drawHeader(aApp, document);
        const TimelineDesc* timelinePointer = document.findTimeline(state.timelineId);
        if (!timelinePointer)
        {
            ImGui::TextDisabled("Create a timeline to animate objects, the camera or to schedule events.");
            ImGui::End();
            return;
        }
        const TimelineDesc timeline = *timelinePointer;
        drawTransport(aApp, document, timeline);

        const float tableHeight = std::max(80.0f, ImGui::GetContentRegionAvail().y - kInspectorHeight);
        int removeTrack = -1;
        int toggleMute = -1;
        if (ImGui::BeginTable("##tracks", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0.0f, tableHeight)))
        {
            ImGui::TableSetupColumn("Track", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
            ImGui::TableSetupColumn("Lane", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow(ImGuiTableRowFlags_None, kRowHeight);
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%zu tracks", timeline.tracks.size());
            ImGui::TableSetColumnIndex(1);
            LaneGeometry lane;
            lane.x0 = ImGui::GetCursorScreenPos().x + 6.0f;
            lane.width = std::max(10.0f, ImGui::GetContentRegionAvail().x - 14.0f);
            lane.duration = std::max(0.1f, timeline.duration);
            ImGui::SetCursorScreenPos(ImVec2(lane.x0, ImGui::GetCursorScreenPos().y));
            drawRuler(aApp, lane, lane.duration);

            for (size_t index = 0; index < timeline.tracks.size(); ++index)
            {
                const TrackDesc& track = timeline.tracks[index];
                ImGui::PushID(static_cast<int>(index));
                ImGui::TableNextRow(ImGuiTableRowFlags_None, kRowHeight);
                ImGui::TableSetColumnIndex(0);
                const bool selected = state.selectedTrack == static_cast<int>(index);
                if (track.muted)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::kDimText);
                }
                if (ImGui::Selectable(trackLabel(document, track).c_str(), selected, ImGuiSelectableFlags_AllowOverlap, ImVec2(kLabelColumnWidth - 90.0f, kRowHeight - 4.0f)))
                {
                    state.selectedTrack = static_cast<int>(index);
                    state.selectedItem = track.kind == TrackKind::Path ? 0 : -1;
                    if (track.target.isObject())
                    {
                        aApp.select(track.target.uid);
                    }
                }
                if (track.muted)
                {
                    ImGui::PopStyleColor();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(track.muted ? "M*" : "M"))
                {
                    toggleMute = static_cast<int>(index);
                }
                ImGui::SetItemTooltip("Mute track");
                if (track.kind == TrackKind::Property)
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("+K"))
                    {
                        std::vector<TimelineDesc> timelines = document.timelines;
                        TrackDesc& editable = findTimeline(timelines, timeline.id)->tracks[index];
                        const float time = state.playhead;
                        auto existing = std::find_if(editable.keys.begin(), editable.keys.end(), [time](const Keyframe& aKey)
                        {
                            return std::abs(aKey.time - time) < 1e-3f;
                        });
                        const Value value = currentTrackValue(aApp, track, time);
                        if (existing == editable.keys.end())
                        {
                            editable.keys.push_back({time, value, {}});
                            sortKeyframes(editable.keys);
                            commit(aApp, std::move(timelines), "Add key");
                        }
                    }
                    ImGui::SetItemTooltip("Add key at playhead");
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("x"))
                {
                    removeTrack = static_cast<int>(index);
                }
                ImGui::SetItemTooltip("Delete track");

                ImGui::TableSetColumnIndex(1);
                ImGui::SetCursorScreenPos(ImVec2(lane.x0, ImGui::GetCursorScreenPos().y));
                drawLane(aApp, document, timeline, index, lane);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (removeTrack >= 0 || toggleMute >= 0)
        {
            std::vector<TimelineDesc> timelines = document.timelines;
            TimelineDesc* editable = findTimeline(timelines, timeline.id);
            if (removeTrack >= 0)
            {
                editable->tracks.erase(editable->tracks.begin() + removeTrack);
                state.selectedTrack = -1;
                state.selectedItem = -1;
                commit(aApp, std::move(timelines), "Delete track");
            }
            else
            {
                editable->tracks[static_cast<size_t>(toggleMute)].muted = !editable->tracks[static_cast<size_t>(toggleMute)].muted;
                commit(aApp, std::move(timelines), "Toggle mute");
            }
        }

        ImGui::Separator();
        drawItemInspector(aApp, document, timeline);
        ImGui::End();
    }
}
