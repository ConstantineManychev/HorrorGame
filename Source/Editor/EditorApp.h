#pragma once

#include "Core/Edit/EditSession.h"
#include "Core/Gameplay/CharacterMotor.h"
#include "Runtime/Animation/TimelinePlayer.h"
#include "Runtime/App/Log.h"
#include "Runtime/Scene/SceneView.h"

#include "ImGui/imgui.h"
#include "axmol.h"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hg
{
    class AppContext;
    class PlayScene;

    enum class GizmoMode
    {
        Move,
        Rotate,
        Scale
    };

    enum class ViewportDrag
    {
        None,
        Pan,
        Move,
        MoveX,
        MoveY,
        Rotate,
        Scale,
        ScaleX,
        ScaleY,
        BoxSelect,
        PathPoint
    };

    struct EditorCamera
    {
        Vec2 center{1920.0f, 1080.0f};
        float zoom = 0.6f;
    };

    struct ViewportState
    {
        GizmoMode gizmo = GizmoMode::Move;
        bool snap = false;
        float gridSize = 50.0f;
        float rotationSnap = 15.0f;
        bool showGrid = true;
        bool showHelpers = true;
        bool showCameraFrame = true;
        bool parallax = false;
        ImVec2 rectMin;
        ImVec2 rectMax;
        bool hovered = false;
        ViewportDrag drag = ViewportDrag::None;
        ImVec2 dragStartMouse;
        Vec2 dragStartWorld;
        float dragStartAngle = 0.0f;
        float dragStartDistance = 1.0f;
        std::vector<std::tuple<ObjectId, Vec2, float, Vec2>> dragInitial;
        size_t dragPathPoint = 0;
    };

    struct TimelineState
    {
        std::string timelineId;
        float playhead = 0.0f;
        bool playing = false;
        bool previewActive = false;
        bool recording = false;
        bool viewThroughCamera = false;
        float speed = 1.0f;
        int selectedTrack = -1;
        int selectedItem = -1;
        float pixelsPerSecond = 140.0f;
        bool draggingItem = false;
        bool draggingClipEdge = false;
        float dragGrabOffset = 0.0f;
    };

    struct PendingAction
    {
        enum class Kind
        {
            None,
            OpenScene,
            NewScene
        };

        Kind kind = Kind::None;
        std::string sceneId;
        SceneKind sceneKind = SceneKind::Location;
    };

    class EditorApp : public TimelineTarget
    {
    public:
        EditorApp(AppContext& aContext, ax::Scene& aScene);
        ~EditorApp() override;

        EditorApp(const EditorApp&) = delete;
        EditorApp& operator=(const EditorApp&) = delete;

        void update(float aDelta);
        void drawGui();

        AppContext& context();
        const SchemaRegistry& schema() const;
        SceneView& view();
        EditSession* session();
        const SceneDocument* document() const;

        void requestOpenScene(const std::string& aId);
        void requestNewScene(const std::string& aId, SceneKind aKind);
        bool openScene(const std::string& aId);
        bool createScene(const std::string& aId, SceneKind aKind);
        bool saveScene();
        void reloadScene();
        bool deleteScene(const std::string& aId);
        const std::vector<std::string>& sceneIds();
        void refreshContentLists();
        bool isDirty() const;

        bool execute(std::unique_ptr<Command> aCommand, bool aMerge = false);
        void undo();
        void redo();
        void endGesture();

        const std::vector<ObjectId>& selection() const;
        ObjectId primarySelection() const;
        bool isSelected(ObjectId aUid) const;
        void select(ObjectId aUid, bool aAdditive = false);
        void setSelection(std::vector<ObjectId> aSelection);
        void clearSelection();
        ObjectId selectableOwner(ObjectId aUid) const;

        ObjectId createObject(const std::string& aType, ObjectId aParent, std::optional<Vec2> aWorldPosition, ValueObject aProps = {});
        ObjectId createPrefabInstance(const std::string& aPrefab, ObjectId aParent, std::optional<Vec2> aWorldPosition);
        void deleteSelection();
        void duplicateSelection();
        void copySelection();
        void paste();
        void renameObject(ObjectId aUid, const std::string& aName);
        void moveObject(ObjectId aUid, ObjectId aParent, size_t aIndex);

        void setObjectProperty(ObjectId aUid, const std::string& aProperty, std::optional<Value> aValue, bool aMerge);
        void setObjectProperties(const std::vector<std::tuple<ObjectId, std::string, Value>>& aChanges, const std::string& aLabel, bool aMerge);
        const Value* effectiveProperty(ObjectId aUid, const std::string& aProperty) const;
        std::string objectType(ObjectId aUid) const;
        const ValueObject* prefabProps(ObjectId aUid) const;

        bool isHidden(ObjectId aUid) const;
        void setHidden(ObjectId aUid, bool aHidden);
        bool isLocked(ObjectId aUid) const;
        void setLocked(ObjectId aUid, bool aLocked);

        EditorCamera& camera();
        ViewportState& viewport();
        void applyCamera();
        void frameObjects(const std::vector<ObjectId>& aObjects);
        void frameAll();
        void frameAllWhenReady();
        void onViewportReady();
        unsigned int dockspaceId() const;
        void centerViewportOn(const Vec2& aWorld);
        Vec2 viewportCenterWorld() const;
        Vec2 imguiToWorld(const ImVec2& aPoint) const;
        ImVec2 worldToImGui(const Vec2& aWorld) const;
        float pixelsPerWorldUnit() const;
        CameraState gameCameraPreview() const;

        TimelineState& timelineState();
        const TimelineDesc* selectedTimeline() const;
        void setTimelines(std::vector<TimelineDesc> aTimelines, const std::string& aLabel, const std::string& aMergeKey = {});
        void startPreview();
        void stopPreview();
        void refreshPreview();
        void seekPreview(float aTime);
        void playPreview();
        void pausePreview();
        bool recordKeys(const std::vector<std::tuple<ObjectId, std::string, Value>>& aChanges, bool aMerge);

        void startPlay(std::optional<Vec2> aSpawnOverride);
        void stopPlay();
        bool isPlaying() const;
        bool& playDebugDraw();

        void validate();
        void log(Severity aSeverity, const std::string& aText);
        const std::deque<LogEntry>& logEntries() const;
        void clearLog();

        const std::vector<std::string>& assetFiles();
        const std::vector<std::string>& prefabFiles();
        const std::vector<std::string>& atlasFrames();

        PendingAction& pendingAction();

        void applyObjectProperty(ObjectId aUid, const std::string& aProperty, const Value& aValue) override;
        void applyCameraProperty(const std::string& aProperty, const Value& aValue) override;
        void placeObjectOnPath(ObjectId aUid, const Vec2& aWorldPosition, std::optional<float> aWorldRotation) override;
        const PathSampler* worldPath(ObjectId aPathUid) override;
        void runTimelineEvent(const ValueObject& aAction) override;

    private:
        void attachSession(SceneDocument aDocument);
        void onDocumentChanged(const ChangeSet& aChanges);
        void rebuildView();
        void pruneSelection();
        void loadAtlasFrames();
        void drawPlayOverlay();

        AppContext& mContext;
        ax::Scene& mScene;
        SceneView mView;
        std::unique_ptr<EditSession> mSession;
        std::vector<ObjectId> mSelection;
        std::unordered_set<ObjectId> mHidden;
        std::unordered_set<ObjectId> mLocked;
        EditorCamera mCamera;
        ViewportState mViewport;
        TimelineState mTimeline;
        std::unique_ptr<TimelinePlayer> mPreviewPlayer;
        CameraState mPreviewCamera;
        std::unordered_map<ObjectId, PathSampler> mPreviewPaths;
        std::deque<LogEntry> mLog;
        size_t mLogListener = 0;
        std::vector<ObjectDesc> mClipboard;
        PlayScene* mPlayScene = nullptr;
        bool mPlayDebugDraw = true;
        PendingAction mPending;
        std::optional<SceneDocument> mPlaySnapshot;
        std::vector<std::string> mSceneIds;
        std::vector<std::string> mAssetFiles;
        std::vector<std::string> mPrefabFiles;
        std::vector<std::string> mAtlasFrames;
        bool mLayoutInitialized = false;
        bool mFramePending = true;
        bool mFocusTimelinePending = false;
        unsigned int mDockspaceId = 0;
    };
}
