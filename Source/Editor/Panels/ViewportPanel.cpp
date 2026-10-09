#include "Editor/Panels/Panels.h"

#include "Core/Base/ValueConvert.h"
#include "Editor/EditorApp.h"
#include "Editor/EditorGui.h"
#include "Runtime/Scene/AxConvert.h"

#include "ImGui/imgui_internal.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <optional>

namespace hg
{
    namespace
    {
        constexpr float kGizmoLength = 70.0f;
        constexpr float kGizmoHitDistance = 7.0f;
        constexpr float kIconRadius = 10.0f;
        constexpr float kPointRadius = 6.0f;

        using Quad = std::array<ImVec2, 4>;

        int gDragButton = 0;

        float distanceToSegment(const ImVec2& aPoint, const ImVec2& aA, const ImVec2& aB)
        {
            const ImVec2 ab{aB.x - aA.x, aB.y - aA.y};
            const ImVec2 ap{aPoint.x - aA.x, aPoint.y - aA.y};
            const float lengthSquared = ab.x * ab.x + ab.y * ab.y;
            const float t = lengthSquared > 0.0f ? std::clamp((ap.x * ab.x + ap.y * ab.y) / lengthSquared, 0.0f, 1.0f) : 0.0f;
            const float dx = aA.x + ab.x * t - aPoint.x;
            const float dy = aA.y + ab.y * t - aPoint.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        float distance(const ImVec2& aA, const ImVec2& aB)
        {
            return std::sqrt((aA.x - aB.x) * (aA.x - aB.x) + (aA.y - aB.y) * (aA.y - aB.y));
        }

        bool quadContains(const Quad& aQuad, const ImVec2& aPoint)
        {
            bool positive = false;
            bool negative = false;
            for (size_t index = 0; index < 4; ++index)
            {
                const ImVec2& a = aQuad[index];
                const ImVec2& b = aQuad[(index + 1) % 4];
                const float cross = (b.x - a.x) * (aPoint.y - a.y) - (b.y - a.y) * (aPoint.x - a.x);
                positive = positive || cross > 0.0f;
                negative = negative || cross < 0.0f;
            }
            return !(positive && negative);
        }

        float quadArea(const Quad& aQuad)
        {
            float area = 0.0f;
            for (size_t index = 0; index < 4; ++index)
            {
                const ImVec2& a = aQuad[index];
                const ImVec2& b = aQuad[(index + 1) % 4];
                area += a.x * b.y - b.x * a.y;
            }
            return std::abs(area) * 0.5f;
        }

        std::optional<Quad> nodeQuad(ax::Node* aNode)
        {
            if (!aNode)
            {
                return std::nullopt;
            }
            const ax::Size size = aNode->getContentSize();
            if (size.width <= 0.0f || size.height <= 0.0f)
            {
                return std::nullopt;
            }
            const std::array<ax::Vec2, 4> corners{ax::Vec2(0.0f, 0.0f), ax::Vec2(size.width, 0.0f), ax::Vec2(size.width, size.height), ax::Vec2(0.0f, size.height)};
            Quad quad;
            for (size_t index = 0; index < 4; ++index)
            {
                quad[index] = sceneToImGui(fromAx(aNode->convertToWorldSpace(corners[index])));
            }
            return quad;
        }

        bool isNodeVisible(ax::Node* aNode, ax::Node* aRoot)
        {
            for (ax::Node* current = aNode; current && current != aRoot; current = current->getParent())
            {
                if (!current->isVisible())
                {
                    return false;
                }
            }
            return true;
        }

        Vec2 snapVector(const Vec2& aValue, float aGrid)
        {
            if (aGrid <= 0.0f)
            {
                return aValue;
            }
            return {std::round(aValue.x / aGrid) * aGrid, std::round(aValue.y / aGrid) * aGrid};
        }

        bool isHelperType(const std::string& aType)
        {
            return aType == "SpawnPoint" || aType == "Path" || aType == "Group" || aType == "ParallaxLayer";
        }

        ObjectId pickObject(EditorApp& aApp, const ImVec2& aMouse)
        {
            SceneView& view = aApp.view();
            ObjectId best = kInvalidObjectId;
            float bestScore = 1e30f;
            view.forEach([&](const SceneEntry& aEntry)
            {
                const ObjectId owner = aApp.selectableOwner(aEntry.uid);
                if (owner == kInvalidObjectId || aApp.isHidden(owner) || aApp.isLocked(owner) || !isNodeVisible(aEntry.node, view.worldLayer()))
                {
                    return;
                }
                float score = 1e30f;
                if (aEntry.type == "Path")
                {
                    if (const PathSampler* sampler = aApp.worldPath(aEntry.uid))
                    {
                        const auto& points = sampler->polyline();
                        for (size_t index = 1; index < points.size(); ++index)
                        {
                            if (distanceToSegment(aMouse, aApp.worldToImGui(points[index - 1]), aApp.worldToImGui(points[index])) < kGizmoHitDistance)
                            {
                                score = 2.0f;
                            }
                        }
                    }
                }
                if (auto quad = nodeQuad(aEntry.node))
                {
                    if (quadContains(*quad, aMouse))
                    {
                        score = std::min(score, 10.0f + quadArea(*quad));
                    }
                }
                else if (isHelperType(aEntry.type) && aEntry.uid == owner)
                {
                    if (distance(aMouse, aApp.worldToImGui(view.worldPositionOf(aEntry.uid))) <= kIconRadius + 2.0f)
                    {
                        score = 1.0f;
                    }
                }
                if (score < bestScore)
                {
                    bestScore = score;
                    best = owner;
                }
            });
            return best;
        }

        Vec2 localValue(EditorApp& aApp, ObjectId aUid, const char* aProperty, const Vec2& aFallback)
        {
            const Value* value = aApp.effectiveProperty(aUid, aProperty);
            return value ? readVec2(*value, aFallback) : aFallback;
        }

        void captureDragInitial(EditorApp& aApp, const ImVec2& aMouse, ViewportDrag aDrag)
        {
            ViewportState& viewport = aApp.viewport();
            viewport.drag = aDrag;
            viewport.dragStartMouse = aMouse;
            viewport.dragStartWorld = aApp.imguiToWorld(aMouse);
            viewport.dragInitial.clear();
            for (ObjectId uid : aApp.selection())
            {
                const Value* rotation = aApp.effectiveProperty(uid, "rotation");
                viewport.dragInitial.emplace_back(uid, aApp.view().worldPositionOf(uid), rotation ? rotation->asFloat() : 0.0f, localValue(aApp, uid, "scale", {1.0f, 1.0f}));
            }
            const ImVec2 pivot = aApp.worldToImGui(aApp.view().worldPositionOf(aApp.primarySelection()));
            viewport.dragStartAngle = radiansToDegrees(std::atan2(aMouse.y - pivot.y, aMouse.x - pivot.x));
            viewport.dragStartDistance = std::max(1.0f, distance(aMouse, pivot));
        }

        std::optional<ViewportDrag> hitGizmo(EditorApp& aApp, const ImVec2& aMouse)
        {
            const ObjectId primary = aApp.primarySelection();
            if (primary == kInvalidObjectId || aApp.isLocked(primary))
            {
                return std::nullopt;
            }
            const ImVec2 pivot = aApp.worldToImGui(aApp.view().worldPositionOf(primary));
            const ImVec2 xEnd{pivot.x + kGizmoLength, pivot.y};
            const ImVec2 yEnd{pivot.x, pivot.y - kGizmoLength};
            switch (aApp.viewport().gizmo)
            {
            case GizmoMode::Move:
                if (std::abs(aMouse.x - pivot.x) <= 7.0f && std::abs(aMouse.y - pivot.y) <= 7.0f)
                {
                    return ViewportDrag::Move;
                }
                if (distanceToSegment(aMouse, pivot, xEnd) < kGizmoHitDistance)
                {
                    return ViewportDrag::MoveX;
                }
                if (distanceToSegment(aMouse, pivot, yEnd) < kGizmoHitDistance)
                {
                    return ViewportDrag::MoveY;
                }
                break;
            case GizmoMode::Rotate:
                if (std::abs(distance(aMouse, pivot) - kGizmoLength) < kGizmoHitDistance)
                {
                    return ViewportDrag::Rotate;
                }
                break;
            case GizmoMode::Scale:
                if (std::abs(aMouse.x - pivot.x) <= 8.0f && std::abs(aMouse.y - pivot.y) <= 8.0f)
                {
                    return ViewportDrag::Scale;
                }
                if (distanceToSegment(aMouse, pivot, xEnd) < kGizmoHitDistance)
                {
                    return ViewportDrag::ScaleX;
                }
                if (distanceToSegment(aMouse, pivot, yEnd) < kGizmoHitDistance)
                {
                    return ViewportDrag::ScaleY;
                }
                break;
            }
            return std::nullopt;
        }

        void drawGizmo(EditorApp& aApp, ImDrawList* aDraw)
        {
            const ObjectId primary = aApp.primarySelection();
            if (primary == kInvalidObjectId || aApp.isLocked(primary))
            {
                return;
            }
            const ImVec2 pivot = aApp.worldToImGui(aApp.view().worldPositionOf(primary));
            const ImVec2 xEnd{pivot.x + kGizmoLength, pivot.y};
            const ImVec2 yEnd{pivot.x, pivot.y - kGizmoLength};
            const ViewportDrag drag = aApp.viewport().drag;
            switch (aApp.viewport().gizmo)
            {
            case GizmoMode::Move:
                aDraw->AddLine(pivot, xEnd, drag == ViewportDrag::MoveX ? EditorColors::kHighlight : EditorColors::kAxisX, 3.0f);
                aDraw->AddTriangleFilled(ImVec2(xEnd.x + 10.0f, xEnd.y), ImVec2(xEnd.x, xEnd.y - 6.0f), ImVec2(xEnd.x, xEnd.y + 6.0f), EditorColors::kAxisX);
                aDraw->AddLine(pivot, yEnd, drag == ViewportDrag::MoveY ? EditorColors::kHighlight : EditorColors::kAxisY, 3.0f);
                aDraw->AddTriangleFilled(ImVec2(yEnd.x, yEnd.y - 10.0f), ImVec2(yEnd.x - 6.0f, yEnd.y), ImVec2(yEnd.x + 6.0f, yEnd.y), EditorColors::kAxisY);
                aDraw->AddRectFilled(ImVec2(pivot.x - 6.0f, pivot.y - 6.0f), ImVec2(pivot.x + 6.0f, pivot.y + 6.0f), drag == ViewportDrag::Move ? EditorColors::kHighlight : EditorColors::kSelection);
                break;
            case GizmoMode::Rotate:
                aDraw->AddCircle(pivot, kGizmoLength, drag == ViewportDrag::Rotate ? EditorColors::kHighlight : EditorColors::kRotate, 48, 3.0f);
                aDraw->AddCircleFilled(pivot, 4.0f, EditorColors::kRotate);
                break;
            case GizmoMode::Scale:
                aDraw->AddLine(pivot, xEnd, drag == ViewportDrag::ScaleX ? EditorColors::kHighlight : EditorColors::kAxisX, 3.0f);
                aDraw->AddRectFilled(ImVec2(xEnd.x - 5.0f, xEnd.y - 5.0f), ImVec2(xEnd.x + 5.0f, xEnd.y + 5.0f), EditorColors::kAxisX);
                aDraw->AddLine(pivot, yEnd, drag == ViewportDrag::ScaleY ? EditorColors::kHighlight : EditorColors::kAxisY, 3.0f);
                aDraw->AddRectFilled(ImVec2(yEnd.x - 5.0f, yEnd.y - 5.0f), ImVec2(yEnd.x + 5.0f, yEnd.y + 5.0f), EditorColors::kAxisY);
                aDraw->AddRectFilled(ImVec2(pivot.x - 7.0f, pivot.y - 7.0f), ImVec2(pivot.x + 7.0f, pivot.y + 7.0f), drag == ViewportDrag::Scale ? EditorColors::kHighlight : EditorColors::kSelection);
                break;
            }
        }

        void drawGrid(EditorApp& aApp, ImDrawList* aDraw)
        {
            ViewportState& viewport = aApp.viewport();
            const float pixelsPerUnit = aApp.pixelsPerWorldUnit();
            float spacing = std::max(1.0f, viewport.gridSize);
            while (spacing * pixelsPerUnit < 12.0f)
            {
                spacing *= 5.0f;
            }
            const Vec2 a = aApp.imguiToWorld(viewport.rectMin);
            const Vec2 b = aApp.imguiToWorld(viewport.rectMax);
            const Vec2 low{std::min(a.x, b.x), std::min(a.y, b.y)};
            const Vec2 high{std::max(a.x, b.x), std::max(a.y, b.y)};
            int lines = 0;
            for (float x = std::floor(low.x / spacing) * spacing; x <= high.x && lines < 400; x += spacing, ++lines)
            {
                aDraw->AddLine(aApp.worldToImGui({x, low.y}), aApp.worldToImGui({x, high.y}), EditorColors::kGrid);
            }
            for (float y = std::floor(low.y / spacing) * spacing; y <= high.y && lines < 800; y += spacing, ++lines)
            {
                aDraw->AddLine(aApp.worldToImGui({low.x, y}), aApp.worldToImGui({high.x, y}), EditorColors::kGrid);
            }
        }

        void addWorldRect(EditorApp& aApp, ImDrawList* aDraw, const Vec2& aLow, const Vec2& aHigh, ImU32 aColor, float aThickness)
        {
            const ImVec2 a = aApp.worldToImGui(aLow);
            const ImVec2 b = aApp.worldToImGui(aHigh);
            aDraw->AddRect(ImVec2(std::min(a.x, b.x), std::min(a.y, b.y)), ImVec2(std::max(a.x, b.x), std::max(a.y, b.y)), aColor, 0.0f, 0, aThickness);
        }

        void drawHelpers(EditorApp& aApp, ImDrawList* aDraw)
        {
            SceneView& view = aApp.view();
            const ObjectId primary = aApp.primarySelection();
            view.forEach([&](const SceneEntry& aEntry)
            {
                if (aApp.isHidden(aApp.selectableOwner(aEntry.uid)))
                {
                    return;
                }
                const ImVec2 position = aApp.worldToImGui(view.worldPositionOf(aEntry.uid));
                if (aEntry.type == "SpawnPoint")
                {
                    aDraw->AddCircleFilled(position, kIconRadius, IM_COL32(40, 120, 60, 200));
                    aDraw->AddCircle(position, kIconRadius, EditorColors::kSpawn, 16, 2.0f);
                    aDraw->AddText(ImVec2(position.x - 4.0f, position.y - 7.0f), EditorColors::kHighlight, "S");
                    aDraw->AddText(ImVec2(position.x + 14.0f, position.y - 7.0f), EditorColors::kSpawn, aEntry.name.c_str());
                }
                else if (aEntry.type == "Path")
                {
                    if (const PathSampler* sampler = aApp.worldPath(aEntry.uid))
                    {
                        std::vector<ImVec2> points;
                        for (const auto& point : sampler->polyline())
                        {
                            points.push_back(aApp.worldToImGui(point));
                        }
                        if (points.size() > 1)
                        {
                            aDraw->AddPolyline(points.data(), static_cast<int>(points.size()), EditorColors::kPath, ImDrawFlags_None, aEntry.uid == primary ? 3.0f : 2.0f);
                        }
                    }
                    const PathShape shape = readPathShape(aEntry.props);
                    for (size_t index = 0; index < shape.points.size(); ++index)
                    {
                        const ImVec2 point = aApp.worldToImGui(view.localToWorld(aEntry.uid, shape.points[index].position));
                        const float radius = aEntry.uid == primary ? kPointRadius : 3.0f;
                        aDraw->AddRectFilled(ImVec2(point.x - radius, point.y - radius), ImVec2(point.x + radius, point.y + radius), index == 0 ? EditorColors::kSpawn : EditorColors::kPath);
                    }
                    if (!shape.points.empty())
                    {
                        const ImVec2 first = aApp.worldToImGui(view.localToWorld(aEntry.uid, shape.points.front().position));
                        aDraw->AddText(ImVec2(first.x + 8.0f, first.y + 4.0f), EditorColors::kPath, aEntry.name.c_str());
                    }
                }
                else if (aEntry.type == "Trigger" || aEntry.type == "Collider")
                {
                    if (auto quad = nodeQuad(aEntry.node))
                    {
                        const bool trigger = aEntry.type == "Trigger";
                        const bool oneWay = aEntry.props.get("one_way").asBool(false);
                        const ImU32 color = trigger ? EditorColors::kTrigger : (oneWay ? EditorColors::kColliderOneWay : EditorColors::kCollider);
                        aDraw->AddQuadFilled((*quad)[0], (*quad)[1], (*quad)[2], (*quad)[3], trigger ? EditorColors::kTriggerFill : IM_COL32(255, 80, 80, 25));
                        aDraw->AddQuad((*quad)[0], (*quad)[1], (*quad)[2], (*quad)[3], color, 2.0f);
                        aDraw->AddText(ImVec2((*quad)[3].x + 4.0f, (*quad)[3].y + 2.0f), color, aEntry.name.c_str());
                    }
                }
                else if (aEntry.type == "ParallaxLayer")
                {
                    aDraw->AddLine(ImVec2(position.x - 8.0f, position.y), ImVec2(position.x + 8.0f, position.y), EditorColors::kHighlight);
                    aDraw->AddLine(ImVec2(position.x, position.y - 8.0f), ImVec2(position.x, position.y + 8.0f), EditorColors::kHighlight);
                }
            });
        }

        void drawSelection(EditorApp& aApp, ImDrawList* aDraw)
        {
            for (ObjectId uid : aApp.selection())
            {
                const ImU32 color = uid == aApp.primarySelection() ? EditorColors::kSelection : EditorColors::kSelectionSecondary;
                ax::Node* node = aApp.view().findNode(uid);
                if (auto quad = nodeQuad(node))
                {
                    aDraw->AddQuad((*quad)[0], (*quad)[1], (*quad)[2], (*quad)[3], color, 2.0f);
                }
                else
                {
                    aDraw->AddCircle(aApp.worldToImGui(aApp.view().worldPositionOf(uid)), kIconRadius + 4.0f, color, 20, 2.0f);
                }
            }
        }

        void drawCameraFrame(EditorApp& aApp, ImDrawList* aDraw)
        {
            const CameraState camera = aApp.gameCameraPreview();
            const Vec2 half = aApp.view().visibleWorldSize(camera.zoom) * 0.5f;
            addWorldRect(aApp, aDraw, camera.center - half, camera.center + half, EditorColors::kCameraFrame, 2.0f);
            const ImVec2 corner = aApp.worldToImGui({camera.center.x - half.x, camera.center.y + half.y});
            aDraw->AddText(ImVec2(corner.x + 4.0f, corner.y + 2.0f), EditorColors::kCameraFrame, aApp.timelineState().previewActive ? "Game camera (timeline)" : "Game camera");
        }

        std::optional<size_t> hitPathPoint(EditorApp& aApp, const ImVec2& aMouse)
        {
            const ObjectId primary = aApp.primarySelection();
            const SceneEntry* entry = aApp.view().find(primary);
            if (!entry || entry->type != "Path" || aApp.isLocked(primary))
            {
                return std::nullopt;
            }
            const PathShape shape = readPathShape(entry->props);
            for (size_t index = 0; index < shape.points.size(); ++index)
            {
                if (distance(aMouse, aApp.worldToImGui(aApp.view().localToWorld(primary, shape.points[index].position))) <= kPointRadius + 3.0f)
                {
                    return index;
                }
            }
            return std::nullopt;
        }

        Vec2 pathLocalPoint(EditorApp& aApp, ObjectId aPath, const Vec2& aWorld)
        {
            ax::Node* node = aApp.view().findNode(aPath);
            if (!node)
            {
                return aWorld;
            }
            return fromAx(node->convertToNodeSpace(toAx(aApp.view().worldToScreen(aWorld))));
        }

        void setPathPoints(EditorApp& aApp, ObjectId aPath, const std::vector<Vec2>& aPoints, bool aMerge)
        {
            aApp.setObjectProperty(aPath, "points", writeVec2List(aPoints), aMerge);
        }

        std::vector<Vec2> pathPoints(EditorApp& aApp, ObjectId aPath)
        {
            const Value* value = aApp.effectiveProperty(aPath, "points");
            return value ? readVec2List(*value) : std::vector<Vec2>{};
        }

        void applyDrag(EditorApp& aApp, const ImVec2& aMouse)
        {
            ViewportState& viewport = aApp.viewport();
            const Vec2 world = aApp.imguiToWorld(aMouse);
            std::vector<std::tuple<ObjectId, std::string, Value>> changes;
            const ImVec2 pivot = aApp.worldToImGui(aApp.view().worldPositionOf(aApp.primarySelection()));

            switch (viewport.drag)
            {
            case ViewportDrag::Move:
            case ViewportDrag::MoveX:
            case ViewportDrag::MoveY:
            {
                Vec2 delta = world - viewport.dragStartWorld;
                if (viewport.drag == ViewportDrag::MoveX)
                {
                    delta.y = 0.0f;
                }
                if (viewport.drag == ViewportDrag::MoveY)
                {
                    delta.x = 0.0f;
                }
                const bool snap = viewport.snap != ImGui::GetIO().KeyCtrl;
                for (const auto& [uid, initialWorld, rotation, scale] : viewport.dragInitial)
                {
                    Vec2 target = initialWorld + delta;
                    if (snap)
                    {
                        target = snapVector(target, viewport.gridSize);
                    }
                    changes.emplace_back(uid, "position", writeVec2(aApp.view().worldToParentLocal(uid, target)));
                }
                aApp.setObjectProperties(changes, "Move", true);
                break;
            }
            case ViewportDrag::Rotate:
            {
                const float angle = radiansToDegrees(std::atan2(aMouse.y - pivot.y, aMouse.x - pivot.x));
                const float delta = angle - viewport.dragStartAngle;
                const bool snap = viewport.snap != ImGui::GetIO().KeyCtrl;
                for (const auto& [uid, initialWorld, rotation, scale] : viewport.dragInitial)
                {
                    float value = rotation + delta;
                    if (snap)
                    {
                        value = std::round(value / viewport.rotationSnap) * viewport.rotationSnap;
                    }
                    changes.emplace_back(uid, "rotation", Value(value));
                }
                aApp.setObjectProperties(changes, "Rotate", true);
                break;
            }
            case ViewportDrag::Scale:
            case ViewportDrag::ScaleX:
            case ViewportDrag::ScaleY:
            {
                float ratioX = 1.0f;
                float ratioY = 1.0f;
                if (viewport.drag == ViewportDrag::Scale)
                {
                    ratioX = ratioY = distance(aMouse, pivot) / viewport.dragStartDistance;
                }
                else if (viewport.drag == ViewportDrag::ScaleX)
                {
                    const float start = viewport.dragStartMouse.x - pivot.x;
                    ratioX = std::abs(start) > 1.0f ? (aMouse.x - pivot.x) / start : 1.0f;
                }
                else
                {
                    const float start = viewport.dragStartMouse.y - pivot.y;
                    ratioY = std::abs(start) > 1.0f ? (aMouse.y - pivot.y) / start : 1.0f;
                }
                const bool snap = viewport.snap != ImGui::GetIO().KeyCtrl;
                for (const auto& [uid, initialWorld, rotation, scale] : viewport.dragInitial)
                {
                    Vec2 value{scale.x * ratioX, scale.y * ratioY};
                    if (snap)
                    {
                        value = snapVector(value, 0.05f);
                    }
                    changes.emplace_back(uid, "scale", writeVec2(value));
                }
                aApp.setObjectProperties(changes, "Scale", true);
                break;
            }
            case ViewportDrag::PathPoint:
            {
                const ObjectId path = aApp.primarySelection();
                std::vector<Vec2> points = pathPoints(aApp, path);
                if (viewport.dragPathPoint < points.size())
                {
                    Vec2 target = world;
                    if (viewport.snap != ImGui::GetIO().KeyCtrl)
                    {
                        target = snapVector(target, viewport.gridSize);
                    }
                    points[viewport.dragPathPoint] = pathLocalPoint(aApp, path, target);
                    setPathPoints(aApp, path, points, true);
                }
                break;
            }
            default:
                break;
            }
        }

        void finishBoxSelect(EditorApp& aApp, const ImVec2& aMouse)
        {
            ViewportState& viewport = aApp.viewport();
            const ImVec2 low{std::min(aMouse.x, viewport.dragStartMouse.x), std::min(aMouse.y, viewport.dragStartMouse.y)};
            const ImVec2 high{std::max(aMouse.x, viewport.dragStartMouse.x), std::max(aMouse.y, viewport.dragStartMouse.y)};
            if (high.x - low.x < 3.0f && high.y - low.y < 3.0f)
            {
                return;
            }
            std::vector<ObjectId> selection = ImGui::GetIO().KeyCtrl ? aApp.selection() : std::vector<ObjectId>{};
            const SceneDocument* document = aApp.document();
            aApp.view().forEach([&](const SceneEntry& aEntry)
            {
                if (!document || !document->findObject(aEntry.uid) || aApp.isHidden(aEntry.uid) || aApp.isLocked(aEntry.uid))
                {
                    return;
                }
                const ImVec2 position = aApp.worldToImGui(aApp.view().worldPositionOf(aEntry.uid));
                if (position.x >= low.x && position.x <= high.x && position.y >= low.y && position.y <= high.y && std::find(selection.begin(), selection.end(), aEntry.uid) == selection.end())
                {
                    selection.push_back(aEntry.uid);
                }
            });
            aApp.setSelection(selection);
        }

        void zoomAt(EditorApp& aApp, const ImVec2& aMouse, float aFactor)
        {
            EditorCamera& camera = aApp.camera();
            const Vec2 before = aApp.imguiToWorld(aMouse);
            camera.zoom = std::clamp(camera.zoom * aFactor, 0.02f, 20.0f);
            aApp.applyCamera();
            const Vec2 after = aApp.imguiToWorld(aMouse);
            camera.center += before - after;
            aApp.applyCamera();
        }

        void handleDrop(EditorApp& aApp, const ImVec2& aMouse)
        {
            if (!ImGui::BeginDragDropTarget())
            {
                return;
            }
            const Vec2 world = aApp.imguiToWorld(aMouse);
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HG_ASSET"))
            {
                const std::string path(static_cast<const char*>(payload->Data), static_cast<size_t>(payload->DataSize));
                auto endsWith = [&path](const char* aSuffix)
                {
                    const std::string suffix(aSuffix);
                    return path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
                };
                if (endsWith(".json") && path.rfind("prefabs/", 0) == 0)
                {
                    aApp.createPrefabInstance(path, kInvalidObjectId, world);
                }
                else if (endsWith(".png") || endsWith(".jpg") || endsWith(".jpeg") || endsWith(".webp"))
                {
                    aApp.createObject("Sprite", kInvalidObjectId, world, ValueObject{{"texture", Value(path)}});
                }
                else if (endsWith(".ttf") || endsWith(".otf"))
                {
                    aApp.createObject("Label", kInvalidObjectId, world, ValueObject{{"font", Value(path)}});
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HG_FRAME"))
            {
                const std::string frame(static_cast<const char*>(payload->Data), static_cast<size_t>(payload->DataSize));
                aApp.createObject("Sprite", kInvalidObjectId, world, ValueObject{{"frame", Value(frame)}});
            }
            ImGui::EndDragDropTarget();
        }

        void handleMouse(EditorApp& aApp, bool aHovered)
        {
            ImGuiIO& io = ImGui::GetIO();
            ViewportState& viewport = aApp.viewport();
            const ImVec2 mouse = io.MousePos;

            if (aHovered && io.MouseWheel != 0.0f)
            {
                zoomAt(aApp, mouse, std::pow(1.15f, io.MouseWheel));
            }

            if (viewport.drag == ViewportDrag::None && aHovered)
            {
                const bool panWithLeft = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsKeyDown(ImGuiKey_Space);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || panWithLeft)
                {
                    viewport.drag = ViewportDrag::Pan;
                    viewport.dragStartMouse = mouse;
                    gDragButton = ImGui::IsMouseClicked(ImGuiMouseButton_Middle) ? ImGuiMouseButton_Middle : (panWithLeft ? ImGuiMouseButton_Left : ImGuiMouseButton_Right);
                }
                else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    gDragButton = ImGuiMouseButton_Left;
                    if (auto point = hitPathPoint(aApp, mouse))
                    {
                        if (io.KeyAlt)
                        {
                            std::vector<Vec2> points = pathPoints(aApp, aApp.primarySelection());
                            if (points.size() > 2)
                            {
                                points.erase(points.begin() + static_cast<std::ptrdiff_t>(*point));
                                setPathPoints(aApp, aApp.primarySelection(), points, false);
                            }
                        }
                        else
                        {
                            viewport.drag = ViewportDrag::PathPoint;
                            viewport.dragPathPoint = *point;
                            viewport.dragStartMouse = mouse;
                        }
                    }
                    else if (auto gizmo = hitGizmo(aApp, mouse))
                    {
                        captureDragInitial(aApp, mouse, *gizmo);
                    }
                    else if (io.KeyCtrl && aApp.view().find(aApp.primarySelection()) && aApp.view().find(aApp.primarySelection())->type == "Path")
                    {
                        const ObjectId path = aApp.primarySelection();
                        std::vector<Vec2> points = pathPoints(aApp, path);
                        points.push_back(pathLocalPoint(aApp, path, aApp.imguiToWorld(mouse)));
                        setPathPoints(aApp, path, points, false);
                    }
                    else
                    {
                        const ObjectId picked = pickObject(aApp, mouse);
                        if (picked != kInvalidObjectId)
                        {
                            if (io.KeyCtrl)
                            {
                                aApp.select(picked, true);
                            }
                            else if (!aApp.isSelected(picked))
                            {
                                aApp.select(picked);
                            }
                            if (aApp.isSelected(picked))
                            {
                                captureDragInitial(aApp, mouse, ViewportDrag::Move);
                            }
                        }
                        else
                        {
                            if (!io.KeyCtrl)
                            {
                                aApp.clearSelection();
                            }
                            viewport.drag = ViewportDrag::BoxSelect;
                            viewport.dragStartMouse = mouse;
                        }
                    }
                }
            }

            if (viewport.drag == ViewportDrag::None)
            {
                return;
            }

            if (ImGui::IsMouseDown(gDragButton))
            {
                if (viewport.drag == ViewportDrag::Pan)
                {
                    const float pixelsPerUnit = std::max(0.0001f, aApp.pixelsPerWorldUnit());
                    aApp.camera().center.x -= io.MouseDelta.x / pixelsPerUnit;
                    aApp.camera().center.y += io.MouseDelta.y / pixelsPerUnit;
                    aApp.applyCamera();
                }
                else if (viewport.drag != ViewportDrag::BoxSelect)
                {
                    const bool moved = distance(mouse, viewport.dragStartMouse) > 2.0f;
                    if (moved && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
                    {
                        applyDrag(aApp, mouse);
                    }
                }
                return;
            }

            if (viewport.drag == ViewportDrag::BoxSelect)
            {
                finishBoxSelect(aApp, mouse);
            }
            viewport.drag = ViewportDrag::None;
            aApp.endGesture();
        }

        void handleKeyboardNudge(EditorApp& aApp)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantTextInput || aApp.selection().empty())
            {
                return;
            }
            Vec2 delta;
            const float step = io.KeyShift ? 10.0f : 1.0f;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
            {
                delta.x -= step;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            {
                delta.x += step;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            {
                delta.y += step;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            {
                delta.y -= step;
            }
            if (delta == Vec2{})
            {
                return;
            }
            std::vector<std::tuple<ObjectId, std::string, Value>> changes;
            for (ObjectId uid : aApp.selection())
            {
                const Vec2 world = aApp.view().worldPositionOf(uid) + delta;
                changes.emplace_back(uid, "position", writeVec2(aApp.view().worldToParentLocal(uid, world)));
            }
            aApp.setObjectProperties(changes, "Nudge", false);
        }

        void drawToolbar(EditorApp& aApp)
        {
            ViewportState& viewport = aApp.viewport();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->ChannelsSplit(2);
            draw->ChannelsSetCurrent(1);
            ImGui::SetCursorScreenPos(ImVec2(viewport.rectMin.x + 12.0f, viewport.rectMin.y + 10.0f));
            ImGui::BeginGroup();
            const std::pair<GizmoMode, const char*> modes[] = {{GizmoMode::Move, "Move (W)"}, {GizmoMode::Rotate, "Rotate (E)"}, {GizmoMode::Scale, "Scale (R)"}};
            for (const auto& [mode, label] : modes)
            {
                const bool active = viewport.gizmo == mode;
                if (active)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                }
                if (ImGui::Button(label))
                {
                    viewport.gizmo = mode;
                }
                if (active)
                {
                    ImGui::PopStyleColor();
                }
                ImGui::SameLine();
            }
            ImGui::Checkbox("Snap", &viewport.snap);
            ImGui::SetItemTooltip("Hold Ctrl to toggle snapping while dragging");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::DragFloat("##grid", &viewport.gridSize, 1.0f, 1.0f, 2000.0f, "grid %.0f");
            ImGui::SameLine();
            ImGui::Text("zoom %.0f%%", aApp.camera().zoom * 100.0f);
            ImGui::SameLine();
            if (ImGui::Button("Play"))
            {
                aApp.startPlay(std::nullopt);
            }
            ImGui::SetItemTooltip("Play scene from its spawn point (Ctrl+P)");
            ImGui::SameLine();
            if (ImGui::Button("Play here"))
            {
                aApp.startPlay(aApp.viewportCenterWorld());
            }
            ImGui::SetItemTooltip("Spawn the player at the viewport center (Ctrl+Shift+P)");
            ImGui::EndGroup();
            const ImVec2 low = ImGui::GetItemRectMin();
            const ImVec2 high = ImGui::GetItemRectMax();
            draw->ChannelsSetCurrent(0);
            draw->AddRectFilled(ImVec2(low.x - 6.0f, low.y - 5.0f), ImVec2(high.x + 6.0f, high.y + 5.0f), IM_COL32(25, 25, 30, 215), 4.0f);
            draw->ChannelsMerge();
        }

        void drawStatus(EditorApp& aApp, bool aHovered)
        {
            ViewportState& viewport = aApp.viewport();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            std::string status;
            if (aHovered)
            {
                const Vec2 world = aApp.imguiToWorld(ImGui::GetIO().MousePos);
                char buffer[96];
                std::snprintf(buffer, sizeof(buffer), "x %.0f  y %.0f", world.x, world.y);
                status = buffer;
            }
            const TimelineState& timeline = aApp.timelineState();
            if (timeline.previewActive)
            {
                char buffer[128];
                std::snprintf(buffer, sizeof(buffer), "   preview '%s' %.2fs", timeline.timelineId.c_str(), timeline.playhead);
                status += buffer;
            }
            if (timeline.recording)
            {
                status += "   REC";
            }
            draw->AddText(ImVec2(viewport.rectMin.x + 10.0f, viewport.rectMax.y - 22.0f), IM_COL32(230, 230, 230, 220), status.c_str());
            if (timeline.recording)
            {
                draw->AddRect(viewport.rectMin, viewport.rectMax, EditorColors::kRecording, 0.0f, 0, 3.0f);
            }
        }
    }

    void drawViewportPanel(EditorApp& aApp)
    {
        const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(aApp.dockspaceId());
        if (central)
        {
            ImGui::SetNextWindowPos(central->Pos);
            ImGui::SetNextWindowSize(central->Size);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse;
        const bool open = ImGui::Begin(PanelNames::kViewport, nullptr, flags);
        ImGui::PopStyleVar(2);
        if (!open || !aApp.document())
        {
            ImGui::End();
            return;
        }

        ViewportState& viewport = aApp.viewport();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 size = ImGui::GetContentRegionAvail();
        if (size.x < 4.0f || size.y < 4.0f)
        {
            ImGui::End();
            return;
        }
        viewport.rectMin = origin;
        viewport.rectMax = ImVec2(origin.x + size.x, origin.y + size.y);
        aApp.onViewportReady();

        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("##canvas", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        const bool hovered = ImGui::IsItemHovered();
        viewport.hovered = hovered;
        handleDrop(aApp, ImGui::GetIO().MousePos);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->PushClipRect(viewport.rectMin, viewport.rectMax, true);
        if (viewport.showGrid)
        {
            drawGrid(aApp, draw);
        }
        addWorldRect(aApp, draw, {0.0f, 0.0f}, aApp.view().worldSize(), EditorColors::kWorldBounds, 1.5f);
        if (viewport.showHelpers)
        {
            drawHelpers(aApp, draw);
        }
        if (viewport.showCameraFrame)
        {
            drawCameraFrame(aApp, draw);
        }
        drawSelection(aApp, draw);
        drawGizmo(aApp, draw);

        if (!aApp.isPlaying())
        {
            handleMouse(aApp, hovered);
            if (ImGui::IsWindowFocused() || hovered)
            {
                handleKeyboardNudge(aApp);
            }
        }

        if (viewport.drag == ViewportDrag::BoxSelect)
        {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            draw->AddRectFilled(viewport.dragStartMouse, mouse, IM_COL32(255, 170, 40, 30));
            draw->AddRect(viewport.dragStartMouse, mouse, EditorColors::kSelection);
        }
        drawStatus(aApp, hovered);
        draw->PopClipRect();

        drawToolbar(aApp);
        ImGui::End();
    }
}
