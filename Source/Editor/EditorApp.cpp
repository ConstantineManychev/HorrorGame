#include "Editor/EditorApp.h"

#include "Core/Base/Json.h"
#include "Core/Base/ValueConvert.h"
#include "Core/Edit/SceneCommands.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Scene/Validator.h"
#include "Editor/EditorGui.h"
#include "Editor/Panels/Panels.h"
#include "Runtime/App/AppContext.h"
#include "Runtime/Scene/AxConvert.h"
#include "Runtime/Scenes/PlayScene.h"

#include "ImGui/ImGuiPresenter.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>

namespace hg
{
    namespace
    {
        constexpr size_t kMaxLogEntries = 2000;
        constexpr float kKeyTimeTolerance = 1.0f / 120.0f;
        constexpr const char* kPlayOverlayId = "#pie";

        bool isValidSceneId(const std::string& aId)
        {
            if (aId.empty())
            {
                return false;
            }
            return std::all_of(aId.begin(), aId.end(), [](unsigned char aCharacter)
            {
                return std::islower(aCharacter) || std::isdigit(aCharacter) || aCharacter == '_';
            });
        }

        std::vector<ObjectId> topLevelOnly(const SceneDocument& aDocument, const std::vector<ObjectId>& aSelection)
        {
            std::vector<ObjectId> result;
            for (ObjectId uid : aSelection)
            {
                const bool nested = std::any_of(aSelection.begin(), aSelection.end(), [&](ObjectId aOther)
                {
                    return aOther != uid && aDocument.isAncestor(aOther, uid);
                });
                if (!nested && aDocument.findObject(uid))
                {
                    result.push_back(uid);
                }
            }
            return result;
        }

        void offsetPosition(ObjectDesc& aObject, const Vec2& aOffset)
        {
            const Vec2 position = readVec2(aObject.props.get("position"), {});
            aObject.props.set("position", writeVec2(position + aOffset));
        }
    }

    EditorApp::EditorApp(AppContext& aContext, ax::Scene& aScene)
        : mContext(aContext)
        , mScene(aScene)
        , mView(aContext, BuildMode::Edit)
    {
        mScene.addChild(mView.root());
        mLogListener = Log::addListener([this](const LogEntry& aEntry)
        {
            mLog.push_back(aEntry);
            while (mLog.size() > kMaxLogEntries)
            {
                mLog.pop_front();
            }
        });

        refreshContentLists();
        loadAtlasFrames();

        const std::string& start = aContext.config().startScene;
        if (!openScene(start))
        {
            if (!mSceneIds.empty())
            {
                openScene(mSceneIds.front());
            }
            else
            {
                createScene("untitled", SceneKind::Location);
            }
        }
        log(Severity::Info, "Editor ready. Content root: " + aContext.content().writableRoot());
    }

    EditorApp::~EditorApp()
    {
        Log::removeListener(mLogListener);
        if (mPlayScene)
        {
            mPlayScene->release();
            mPlayScene = nullptr;
        }
        mPreviewPlayer.reset();
        mSession.reset();
    }

    void EditorApp::update(float aDelta)
    {
        if (mTimeline.previewActive && mTimeline.playing && mPreviewPlayer)
        {
            mPreviewPlayer->setSpeed(mTimeline.speed);
            mPreviewPlayer->update(aDelta);
            mTimeline.playhead = mPreviewPlayer->time();
            if (mPreviewPlayer->isFinished())
            {
                mTimeline.playing = false;
            }
        }
        applyCamera();
    }

    void EditorApp::drawGui()
    {
        drawMainMenu(*this);
        const ImGuiID dockspace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        mDockspaceId = dockspace;
        if (!mLayoutInitialized || mLayoutResetRequested)
        {
            buildDefaultLayout(dockspace, mLayoutResetRequested);
            mLayoutInitialized = true;
            mLayoutResetRequested = false;
            mFocusTimelinePending = true;
        }

        drawScenesPanel(*this);
        drawHierarchyPanel(*this);
        drawInspectorPanel(*this);
        drawTimelinePanel(*this);
        drawAssetsPanel(*this);
        drawConsolePanel(*this);
        drawViewportPanel(*this);
        handleShortcuts(*this);
        drawModals(*this);
        if (mFocusTimelinePending)
        {
            mFocusTimelinePending = false;
            ImGui::SetWindowFocus(PanelNames::kTimeline);
        }
    }

    AppContext& EditorApp::context()
    {
        return mContext;
    }

    const SchemaRegistry& EditorApp::schema() const
    {
        return mContext.schema();
    }

    SceneView& EditorApp::view()
    {
        return mView;
    }

    EditSession* EditorApp::session()
    {
        return mSession.get();
    }

    const SceneDocument* EditorApp::document() const
    {
        return mSession ? &mSession->document() : nullptr;
    }

    void EditorApp::requestOpenScene(const std::string& aId)
    {
        if (isDirty())
        {
            mPending = {PendingAction::Kind::OpenScene, aId, SceneKind::Location};
            return;
        }
        openScene(aId);
    }

    void EditorApp::requestNewScene(const std::string& aId, SceneKind aKind)
    {
        if (isDirty())
        {
            mPending = {PendingAction::Kind::NewScene, aId, aKind};
            return;
        }
        createScene(aId, aKind);
    }

    bool EditorApp::openScene(const std::string& aId)
    {
        if (aId.empty())
        {
            return false;
        }
        Diagnostics diagnostics;
        auto document = mContext.content().loadScene(aId, diagnostics);
        Log::diagnostics(diagnostics);
        if (!document)
        {
            return false;
        }
        mContext.content().prefabs().invalidate();
        attachSession(std::move(*document));
        frameAllWhenReady();
        log(Severity::Info, "Opened scene '" + aId + "'");
        return true;
    }

    bool EditorApp::createScene(const std::string& aId, SceneKind aKind)
    {
        if (!isValidSceneId(aId))
        {
            log(Severity::Error, "Scene id must contain only lowercase letters, digits and '_'");
            return false;
        }
        if (std::find(mSceneIds.begin(), mSceneIds.end(), aId) != mSceneIds.end())
        {
            log(Severity::Error, "Scene '" + aId + "' already exists");
            return false;
        }
        attachSession(createEmptyScene(aId, aKind));
        saveScene();
        frameAllWhenReady();
        return true;
    }

    bool EditorApp::saveScene()
    {
        if (!mSession)
        {
            return false;
        }
        Diagnostics diagnostics;
        if (!mContext.content().saveScene(mSession->document(), diagnostics))
        {
            Log::diagnostics(diagnostics);
            return false;
        }
        mSession->markSaved();
        refreshContentLists();
        log(Severity::Info, "Saved " + sceneFilePath(mSession->document().id));
        validate();
        return true;
    }

    void EditorApp::reloadScene()
    {
        if (mSession)
        {
            const std::string id = mSession->document().id;
            openScene(id);
        }
    }

    bool EditorApp::deleteScene(const std::string& aId)
    {
        if (!mContext.content().deleteScene(aId))
        {
            log(Severity::Error, "Cannot delete scene '" + aId + "'");
            return false;
        }
        log(Severity::Info, "Deleted scene '" + aId + "'");
        refreshContentLists();
        if (mSession && mSession->document().id == aId)
        {
            mSession->markSaved();
            if (!mSceneIds.empty())
            {
                openScene(mSceneIds.front());
            }
            else
            {
                createScene("untitled", SceneKind::Location);
            }
        }
        return true;
    }

    const std::vector<std::string>& EditorApp::sceneIds()
    {
        return mSceneIds;
    }

    void EditorApp::refreshContentLists()
    {
        ContentStore& content = mContext.content();
        mSceneIds = content.listScenes();
        mAssetFiles.clear();
        for (const char* directory : {"res", "fonts", "audio", "sounds", "music"})
        {
            const auto files = content.listFiles(directory, {});
            mAssetFiles.insert(mAssetFiles.end(), files.begin(), files.end());
        }
        mPrefabFiles = content.listFiles("prefabs", {".json"});
    }

    bool EditorApp::isDirty() const
    {
        return mSession && mSession->isDirty();
    }

    bool EditorApp::execute(std::unique_ptr<Command> aCommand, bool aMerge)
    {
        return mSession && mSession->execute(std::move(aCommand), aMerge);
    }

    void EditorApp::undo()
    {
        if (mSession && mSession->undo())
        {
            log(Severity::Info, "Undo");
        }
    }

    void EditorApp::redo()
    {
        if (mSession && mSession->redo())
        {
            log(Severity::Info, "Redo");
        }
    }

    void EditorApp::endGesture()
    {
        if (mSession)
        {
            mSession->closeMergeWindow();
        }
    }

    const std::vector<ObjectId>& EditorApp::selection() const
    {
        return mSelection;
    }

    ObjectId EditorApp::primarySelection() const
    {
        return mSelection.empty() ? kInvalidObjectId : mSelection.back();
    }

    bool EditorApp::isSelected(ObjectId aUid) const
    {
        return std::find(mSelection.begin(), mSelection.end(), aUid) != mSelection.end();
    }

    void EditorApp::select(ObjectId aUid, bool aAdditive)
    {
        if (aUid == kInvalidObjectId)
        {
            if (!aAdditive)
            {
                mSelection.clear();
            }
            return;
        }
        if (!aAdditive)
        {
            mSelection = {aUid};
            return;
        }
        auto it = std::find(mSelection.begin(), mSelection.end(), aUid);
        if (it != mSelection.end())
        {
            mSelection.erase(it);
        }
        else
        {
            mSelection.push_back(aUid);
        }
    }

    void EditorApp::setSelection(std::vector<ObjectId> aSelection)
    {
        mSelection = std::move(aSelection);
        pruneSelection();
    }

    void EditorApp::clearSelection()
    {
        mSelection.clear();
    }

    ObjectId EditorApp::selectableOwner(ObjectId aUid) const
    {
        const SceneDocument* doc = document();
        ObjectId current = aUid;
        for (int depth = 0; depth < 64 && current != kInvalidObjectId; ++depth)
        {
            if (doc && doc->findObject(current))
            {
                return current;
            }
            const SceneEntry* entry = mView.find(current);
            current = entry ? entry->parent : kInvalidObjectId;
        }
        return kInvalidObjectId;
    }

    ObjectId EditorApp::createObject(const std::string& aType, ObjectId aParent, std::optional<Vec2> aWorldPosition, ValueObject aProps)
    {
        if (!mSession)
        {
            return kInvalidObjectId;
        }
        SceneDocument& doc = mSession->mutableDocument();
        std::vector<ObjectDesc>* siblings = doc.childrenOf(aParent);
        if (!siblings)
        {
            aParent = kInvalidObjectId;
            siblings = &doc.objects;
        }

        ObjectDesc object;
        object.uid = doc.allocateUid();
        object.type = aType;
        object.name = makeUniqueName(*siblings, aType);
        object.props = std::move(aProps);
        if (aWorldPosition)
        {
            Vec2 local = *aWorldPosition;
            if (ax::Node* parentNode = mView.findNode(aParent))
            {
                local = fromAx(parentNode->convertToNodeSpace(toAx(mView.worldToScreen(*aWorldPosition))));
            }
            object.props.set("position", writeVec2(local));
        }

        const size_t index = siblings->size();
        if (!execute(std::make_unique<CreateObjectCommand>(aParent, index, object)))
        {
            return kInvalidObjectId;
        }
        select(object.uid);
        return object.uid;
    }

    ObjectId EditorApp::createPrefabInstance(const std::string& aPrefab, ObjectId aParent, std::optional<Vec2> aWorldPosition)
    {
        Diagnostics diagnostics;
        const ObjectDesc* prefab = mContext.content().prefabs().find(aPrefab, diagnostics);
        if (!prefab)
        {
            Log::diagnostics(diagnostics);
            return kInvalidObjectId;
        }
        if (!mSession)
        {
            return kInvalidObjectId;
        }
        SceneDocument& doc = mSession->mutableDocument();
        std::vector<ObjectDesc>* siblings = doc.childrenOf(aParent);
        if (!siblings)
        {
            aParent = kInvalidObjectId;
            siblings = &doc.objects;
        }
        ObjectDesc object;
        object.uid = doc.allocateUid();
        object.prefab = aPrefab;
        object.name = makeUniqueName(*siblings, prefab->name.empty() ? "Prefab" : prefab->name);
        if (aWorldPosition)
        {
            Vec2 local = *aWorldPosition;
            if (ax::Node* parentNode = mView.findNode(aParent))
            {
                local = fromAx(parentNode->convertToNodeSpace(toAx(mView.worldToScreen(*aWorldPosition))));
            }
            object.props.set("position", writeVec2(local));
        }
        if (!execute(std::make_unique<CreateObjectCommand>(aParent, siblings->size(), object)))
        {
            return kInvalidObjectId;
        }
        select(object.uid);
        return object.uid;
    }

    void EditorApp::deleteSelection()
    {
        if (!mSession || mSelection.empty())
        {
            return;
        }
        auto composite = std::make_unique<CompositeCommand>("Delete objects");
        for (ObjectId uid : topLevelOnly(mSession->document(), mSelection))
        {
            composite->add(std::make_unique<DeleteObjectCommand>(uid));
        }
        if (!composite->empty() && execute(std::move(composite)))
        {
            clearSelection();
        }
    }

    void EditorApp::duplicateSelection()
    {
        if (!mSession || mSelection.empty())
        {
            return;
        }
        SceneDocument& doc = mSession->mutableDocument();
        auto composite = std::make_unique<CompositeCommand>("Duplicate objects");
        std::vector<ObjectId> created;
        for (ObjectId uid : topLevelOnly(doc, mSelection))
        {
            const ObjectDesc* original = doc.findObject(uid);
            std::vector<ObjectDesc>* siblings = doc.findSiblings(uid);
            if (!original || !siblings)
            {
                continue;
            }
            ObjectDesc copy = *original;
            reassignUids(copy, doc);
            copy.name = makeUniqueName(*siblings, original->name);
            offsetPosition(copy, {40.0f, -40.0f});
            const ObjectDesc* parent = doc.findParent(uid);
            const size_t index = static_cast<size_t>(original - static_cast<const ObjectDesc*>(siblings->data())) + 1;
            created.push_back(copy.uid);
            composite->add(std::make_unique<CreateObjectCommand>(parent ? parent->uid : kInvalidObjectId, index, std::move(copy)));
        }
        if (!composite->empty() && execute(std::move(composite)))
        {
            setSelection(created);
        }
    }

    void EditorApp::copySelection()
    {
        if (!mSession)
        {
            return;
        }
        mClipboard.clear();
        for (ObjectId uid : topLevelOnly(mSession->document(), mSelection))
        {
            mClipboard.push_back(*mSession->document().findObject(uid));
        }
        log(Severity::Info, "Copied " + std::to_string(mClipboard.size()) + " object(s)");
    }

    void EditorApp::paste()
    {
        if (!mSession || mClipboard.empty())
        {
            return;
        }
        SceneDocument& doc = mSession->mutableDocument();
        ObjectId parent = kInvalidObjectId;
        if (const ObjectDesc* parentDesc = doc.findParent(primarySelection()))
        {
            parent = parentDesc->uid;
        }
        std::vector<ObjectDesc>* siblings = doc.childrenOf(parent);
        auto composite = std::make_unique<CompositeCommand>("Paste objects");
        std::vector<ObjectId> created;
        size_t index = siblings->size();
        for (const auto& source : mClipboard)
        {
            ObjectDesc copy = source;
            reassignUids(copy, doc);
            copy.name = makeUniqueName(*siblings, source.name);
            offsetPosition(copy, {60.0f, -60.0f});
            created.push_back(copy.uid);
            composite->add(std::make_unique<CreateObjectCommand>(parent, index++, std::move(copy)));
        }
        if (execute(std::move(composite)))
        {
            setSelection(created);
        }
    }

    void EditorApp::renameObject(ObjectId aUid, const std::string& aName)
    {
        execute(std::make_unique<RenameObjectCommand>(aUid, aName));
    }

    void EditorApp::moveObject(ObjectId aUid, ObjectId aParent, size_t aIndex)
    {
        if (!mSession)
        {
            return;
        }
        if (aParent != kInvalidObjectId)
        {
            const SchemaRegistry& registry = schema();
            const TypeSchema* type = registry.findType(objectType(aParent));
            if (type && !type->allowsChildren)
            {
                log(Severity::Warning, "Objects of type '" + type->id + "' cannot have children");
                return;
            }
        }
        execute(std::make_unique<MoveObjectCommand>(aUid, aParent, aIndex));
    }

    void EditorApp::setObjectProperty(ObjectId aUid, const std::string& aProperty, std::optional<Value> aValue, bool aMerge)
    {
        if (aValue && mTimeline.recording && recordKeys({{aUid, aProperty, *aValue}}, aMerge))
        {
            return;
        }
        execute(std::make_unique<SetPropertyCommand>(aUid, aProperty, std::move(aValue)), aMerge);
    }

    void EditorApp::setObjectProperties(const std::vector<std::tuple<ObjectId, std::string, Value>>& aChanges, const std::string& aLabel, bool aMerge)
    {
        if (aChanges.empty())
        {
            return;
        }
        if (mTimeline.recording && recordKeys(aChanges, aMerge))
        {
            return;
        }
        auto composite = std::make_unique<CompositeCommand>(aLabel);
        for (const auto& [uid, property, value] : aChanges)
        {
            composite->add(std::make_unique<SetPropertyCommand>(uid, property, value));
        }
        execute(std::move(composite), aMerge);
    }

    const Value* EditorApp::effectiveProperty(ObjectId aUid, const std::string& aProperty) const
    {
        const SceneDocument* doc = document();
        const ObjectDesc* object = doc ? doc->findObject(aUid) : nullptr;
        if (!object)
        {
            const SceneEntry* entry = mView.find(aUid);
            return entry ? entry->props.find(aProperty) : nullptr;
        }
        if (const Value* value = object->props.find(aProperty))
        {
            return value;
        }
        if (const ValueObject* prefab = prefabProps(aUid))
        {
            return prefab->find(aProperty);
        }
        return nullptr;
    }

    std::string EditorApp::objectType(ObjectId aUid) const
    {
        const SceneDocument* doc = document();
        const ObjectDesc* object = doc ? doc->findObject(aUid) : nullptr;
        if (!object)
        {
            const SceneEntry* entry = mView.find(aUid);
            return entry ? entry->type : std::string();
        }
        Diagnostics ignored;
        return effectiveObjectType(*object, &mContext.content().prefabs(), ignored);
    }

    const ValueObject* EditorApp::prefabProps(ObjectId aUid) const
    {
        const SceneDocument* doc = document();
        const ObjectDesc* object = doc ? doc->findObject(aUid) : nullptr;
        if (!object || object->prefab.empty())
        {
            return nullptr;
        }
        Diagnostics ignored;
        const ObjectDesc* prefab = mContext.content().prefabs().find(object->prefab, ignored);
        return prefab ? &prefab->props : nullptr;
    }

    bool EditorApp::isHidden(ObjectId aUid) const
    {
        return mHidden.count(aUid) > 0;
    }

    void EditorApp::setHidden(ObjectId aUid, bool aHidden)
    {
        if (aHidden)
        {
            mHidden.insert(aUid);
        }
        else
        {
            mHidden.erase(aUid);
        }
        if (ax::Node* node = mView.findNode(aUid))
        {
            const Value* visible = effectiveProperty(aUid, "visible");
            node->setVisible(!aHidden && (!visible || visible->asBool(true)));
        }
    }

    bool EditorApp::isLocked(ObjectId aUid) const
    {
        return mLocked.count(aUid) > 0;
    }

    void EditorApp::setLocked(ObjectId aUid, bool aLocked)
    {
        if (aLocked)
        {
            mLocked.insert(aUid);
            mSelection.erase(std::remove(mSelection.begin(), mSelection.end(), aUid), mSelection.end());
        }
        else
        {
            mLocked.erase(aUid);
        }
    }

    EditorCamera& EditorApp::camera()
    {
        return mCamera;
    }

    ViewportState& EditorApp::viewport()
    {
        return mViewport;
    }

    void EditorApp::applyCamera()
    {
        if (mTimeline.previewActive && mTimeline.viewThroughCamera)
        {
            mView.setCamera(mPreviewCamera);
            return;
        }
        CameraState state;
        state.center = mCamera.center;
        state.zoom = mCamera.zoom;
        mView.setCamera(state);
    }

    void EditorApp::frameObjects(const std::vector<ObjectId>& aObjects)
    {
        if (aObjects.empty())
        {
            frameAll();
            return;
        }
        Vec2 low{1e9f, 1e9f};
        Vec2 high{-1e9f, -1e9f};
        for (ObjectId uid : aObjects)
        {
            const Vec2 position = mView.worldPositionOf(uid);
            Vec2 extent{100.0f, 100.0f};
            if (ax::Node* node = mView.findNode(uid))
            {
                const ax::Size size = node->getContentSize();
                extent = {std::max(100.0f, size.width * std::abs(node->getScaleX())), std::max(100.0f, size.height * std::abs(node->getScaleY()))};
            }
            low = {std::min(low.x, position.x - extent.x * 0.5f), std::min(low.y, position.y - extent.y * 0.5f)};
            high = {std::max(high.x, position.x + extent.x * 0.5f), std::max(high.y, position.y + extent.y * 0.5f)};
        }
        const Vec2 size = high - low;
        const ImVec2 viewportSize{mViewport.rectMax.x - mViewport.rectMin.x, mViewport.rectMax.y - mViewport.rectMin.y};
        const float pixelsAtZoomOne = pixelsPerWorldUnit() / std::max(0.0001f, mCamera.zoom);
        if (viewportSize.x > 10.0f && viewportSize.y > 10.0f && pixelsAtZoomOne > 0.0f)
        {
            const float zoom = std::min(viewportSize.x / (size.x * pixelsAtZoomOne), viewportSize.y / (size.y * pixelsAtZoomOne)) * 0.85f;
            mCamera.zoom = std::clamp(zoom, 0.02f, 20.0f);
        }
        applyCamera();
        centerViewportOn((low + high) * 0.5f);
    }

    void EditorApp::frameAll()
    {
        const Vec2 world = mView.worldSize();
        const ImVec2 viewportSize{mViewport.rectMax.x - mViewport.rectMin.x, mViewport.rectMax.y - mViewport.rectMin.y};
        mCamera.center = world * 0.5f;
        const float pixelsAtZoomOne = pixelsPerWorldUnit() / std::max(0.0001f, mCamera.zoom);
        if (viewportSize.x > 10.0f && viewportSize.y > 10.0f && pixelsAtZoomOne > 0.0f)
        {
            mCamera.zoom = std::clamp(std::min(viewportSize.x / (world.x * pixelsAtZoomOne), viewportSize.y / (world.y * pixelsAtZoomOne)) * 0.92f, 0.02f, 20.0f);
        }
        applyCamera();
        centerViewportOn(world * 0.5f);
    }

    void EditorApp::frameAllWhenReady()
    {
        mFramePending = true;
        if (mViewport.rectMax.x > mViewport.rectMin.x + 10.0f)
        {
            onViewportReady();
        }
    }

    void EditorApp::onViewportReady()
    {
        if (mFramePending)
        {
            mFramePending = false;
            frameAll();
        }
    }

    unsigned int EditorApp::dockspaceId() const
    {
        return mDockspaceId;
    }

    void EditorApp::resetLayout()
    {
        mLayoutResetRequested = true;
    }

    void EditorApp::centerViewportOn(const Vec2& aWorld)
    {
        if (mViewport.rectMax.x <= mViewport.rectMin.x)
        {
            mCamera.center = aWorld;
            applyCamera();
            return;
        }
        const Vec2 current = viewportCenterWorld();
        mCamera.center += aWorld - current;
        applyCamera();
    }

    Vec2 EditorApp::viewportCenterWorld() const
    {
        const ImVec2 center{(mViewport.rectMin.x + mViewport.rectMax.x) * 0.5f, (mViewport.rectMin.y + mViewport.rectMax.y) * 0.5f};
        return imguiToWorld(center);
    }

    Vec2 EditorApp::imguiToWorld(const ImVec2& aPoint) const
    {
        return mView.screenToWorld(imguiToScene(aPoint));
    }

    ImVec2 EditorApp::worldToImGui(const Vec2& aWorld) const
    {
        return sceneToImGui(mView.worldToScreen(aWorld));
    }

    float EditorApp::pixelsPerWorldUnit() const
    {
        return mView.worldScale() * sceneToImGuiScale();
    }

    CameraState EditorApp::gameCameraPreview() const
    {
        if (mTimeline.previewActive)
        {
            return mPreviewCamera;
        }
        CameraState state;
        const Vec2 world = mView.worldSize();
        state.center = world * 0.5f;
        if (const SceneDocument* doc = document(); doc && doc->kind == SceneKind::Location)
        {
            mView.forEach([&state, this](const SceneEntry& aEntry)
            {
                if (aEntry.type == "SpawnPoint" && aEntry.props.get("player").asBool(true))
                {
                    state.center = mView.worldPositionOf(aEntry.uid);
                }
            });
        }
        const Vec2 half = mView.visibleWorldSize(1.0f) * 0.5f;
        state.center.x = world.x <= half.x * 2.0f ? world.x * 0.5f : std::clamp(state.center.x, half.x, world.x - half.x);
        state.center.y = world.y <= half.y * 2.0f ? world.y * 0.5f : std::clamp(state.center.y, half.y, world.y - half.y);
        return state;
    }

    TimelineState& EditorApp::timelineState()
    {
        return mTimeline;
    }

    const TimelineDesc* EditorApp::selectedTimeline() const
    {
        const SceneDocument* doc = document();
        return doc ? doc->findTimeline(mTimeline.timelineId) : nullptr;
    }

    void EditorApp::setTimelines(std::vector<TimelineDesc> aTimelines, const std::string& aLabel, const std::string& aMergeKey)
    {
        execute(std::make_unique<SetTimelinesCommand>(std::move(aTimelines), aLabel, aMergeKey), !aMergeKey.empty());
    }

    void EditorApp::startPreview()
    {
        const TimelineDesc* timeline = selectedTimeline();
        if (!timeline)
        {
            return;
        }
        mPreviewCamera = gameCameraPreview();
        mTimeline.previewActive = true;
        mPreviewPaths.clear();
        mPreviewPlayer = std::make_unique<TimelinePlayer>(*timeline, *this);
        mPreviewPlayer->seek(mTimeline.playhead);
    }

    void EditorApp::stopPreview()
    {
        const bool wasActive = mTimeline.previewActive;
        mTimeline.previewActive = false;
        mTimeline.playing = false;
        mPreviewPlayer.reset();
        if (wasActive && mSession)
        {
            rebuildView();
        }
    }

    void EditorApp::refreshPreview()
    {
        if (!mTimeline.previewActive)
        {
            return;
        }
        const TimelineDesc* timeline = selectedTimeline();
        if (!timeline)
        {
            stopPreview();
            return;
        }
        const bool playing = mTimeline.playing;
        mTimeline.previewActive = false;
        rebuildView();
        mPreviewCamera = gameCameraPreview();
        mTimeline.previewActive = true;
        mPreviewPaths.clear();
        mPreviewPlayer = std::make_unique<TimelinePlayer>(*timeline, *this);
        mPreviewPlayer->seek(mTimeline.playhead);
        if (playing)
        {
            mPreviewPlayer->play(mTimeline.playhead);
        }
    }

    void EditorApp::seekPreview(float aTime)
    {
        const TimelineDesc* timeline = selectedTimeline();
        if (!timeline)
        {
            return;
        }
        mTimeline.playhead = std::clamp(aTime, 0.0f, timeline->duration);
        if (!mTimeline.previewActive || !mPreviewPlayer)
        {
            startPreview();
            return;
        }
        const SceneDocument* doc = document();
        for (const auto& track : timeline->tracks)
        {
            if (track.kind == TrackKind::Path && track.target.isObject() && doc)
            {
                for (const char* property : {"position", "rotation"})
                {
                    mView.applyProperty(track.target.uid, property, effectiveProperty(track.target.uid, property));
                }
            }
        }
        mPreviewCamera.shake = 0.0f;
        mPreviewPlayer->seek(mTimeline.playhead);
    }

    void EditorApp::playPreview()
    {
        const TimelineDesc* timeline = selectedTimeline();
        if (!timeline)
        {
            return;
        }
        if (mTimeline.playhead >= timeline->duration - 1e-3f)
        {
            mTimeline.playhead = 0.0f;
        }
        if (!mTimeline.previewActive || !mPreviewPlayer)
        {
            startPreview();
        }
        mPreviewPlayer->play(mTimeline.playhead);
        mTimeline.playing = true;
    }

    void EditorApp::pausePreview()
    {
        mTimeline.playing = false;
        if (mPreviewPlayer)
        {
            mPreviewPlayer->pause();
        }
    }

    bool EditorApp::recordKeys(const std::vector<std::tuple<ObjectId, std::string, Value>>& aChanges, bool aMerge)
    {
        const SceneDocument* doc = document();
        if (!doc || !doc->findTimeline(mTimeline.timelineId))
        {
            return false;
        }
        for (const auto& [uid, property, value] : aChanges)
        {
            const PropertySchema* schemaProperty = schema().findProperty(objectType(uid), property);
            if (!schemaProperty || !schemaProperty->animatable)
            {
                return false;
            }
        }

        std::vector<TimelineDesc> timelines = doc->timelines;
        TimelineDesc* timeline = nullptr;
        for (auto& candidate : timelines)
        {
            if (candidate.id == mTimeline.timelineId)
            {
                timeline = &candidate;
            }
        }
        const float time = mTimeline.playhead;
        for (const auto& [uid, property, value] : aChanges)
        {
            TrackDesc* track = nullptr;
            for (auto& candidate : timeline->tracks)
            {
                if (candidate.kind == TrackKind::Property && candidate.target == TargetRef::object(uid) && candidate.property == property)
                {
                    track = &candidate;
                }
            }
            if (!track)
            {
                TrackDesc created;
                created.target = TargetRef::object(uid);
                created.property = property;
                timeline->tracks.push_back(std::move(created));
                track = &timeline->tracks.back();
            }
            auto existing = std::find_if(track->keys.begin(), track->keys.end(), [time](const Keyframe& aKey)
            {
                return std::abs(aKey.time - time) <= kKeyTimeTolerance;
            });
            if (existing != track->keys.end())
            {
                existing->value = value;
            }
            else
            {
                track->keys.push_back({time, value, {}});
                sortKeyframes(track->keys);
            }
        }
        timeline->duration = std::max(timeline->duration, time);
        setTimelines(std::move(timelines), "Record key", aMerge ? "record" : std::string());
        return true;
    }

    void EditorApp::startPlay(std::optional<Vec2> aSpawnOverride)
    {
        if (mPlayScene || !mSession)
        {
            return;
        }
        stopPreview();
        mPlaySnapshot = mSession->document();

        PlayOptions options;
        options.loadScene = [this](const std::string& aSceneId) -> std::optional<SceneDocument>
        {
            if (mPlaySnapshot && mPlaySnapshot->id == aSceneId)
            {
                return mPlaySnapshot;
            }
            Diagnostics diagnostics;
            auto loaded = mContext.content().loadScene(aSceneId, diagnostics);
            Log::diagnostics(diagnostics);
            return loaded;
        };
        options.onExitRequested = [this]()
        {
            stopPlay();
        };
        options.onWorldStarted = [this](World& aWorld)
        {
            aWorld.setDebugDraw(mPlayDebugDraw);
        };

        mPlayScene = PlayScene::create(mContext, std::move(options));
        mPlayScene->retain();
        mPlayScene->loadDocument(*mPlaySnapshot, aSpawnOverride);
        ax::Director::getInstance()->pushScene(mPlayScene);
        ax::extension::ImGuiPresenter::getInstance()->addRenderLoop(kPlayOverlayId, [this]()
        {
            drawPlayOverlay();
        }, mPlayScene);
        log(Severity::Info, "Play mode started (Esc to stop)");
    }

    void EditorApp::stopPlay()
    {
        if (!mPlayScene)
        {
            return;
        }
        ax::extension::ImGuiPresenter::getInstance()->removeRenderLoop(kPlayOverlayId);
        mContext.audio().stopAll();
        mContext.input().reset();
        ax::Director::getInstance()->popScene();
        mPlayScene->release();
        mPlayScene = nullptr;
        log(Severity::Info, "Play mode stopped");
    }

    bool EditorApp::isPlaying() const
    {
        return mPlayScene != nullptr;
    }

    bool& EditorApp::playDebugDraw()
    {
        return mPlayDebugDraw;
    }

    void EditorApp::validate()
    {
        if (!mSession)
        {
            return;
        }
        ValidationContext validation;
        validation.schema = &schema();
        validation.assetExists = [this](const std::string& aPath)
        {
            return mContext.content().exists(aPath);
        };
        validation.sceneExists = [this](const std::string& aId)
        {
            return std::find(mSceneIds.begin(), mSceneIds.end(), aId) != mSceneIds.end();
        };
        validation.prefabs = &mContext.content().prefabs();

        Diagnostics diagnostics;
        diagnostics.setSource(sceneFilePath(mSession->document().id));
        validateScene(mSession->document(), validation, diagnostics);
        Log::diagnostics(diagnostics);
        log(diagnostics.hasErrors() ? Severity::Error : Severity::Info, "Validation: " + std::to_string(diagnostics.count(Severity::Error)) + " errors, " + std::to_string(diagnostics.count(Severity::Warning)) + " warnings");
    }

    void EditorApp::log(Severity aSeverity, const std::string& aText)
    {
        Log::write(aSeverity, aText);
    }

    const std::deque<LogEntry>& EditorApp::logEntries() const
    {
        return mLog;
    }

    void EditorApp::clearLog()
    {
        mLog.clear();
    }

    const std::vector<std::string>& EditorApp::assetFiles()
    {
        return mAssetFiles;
    }

    const std::vector<std::string>& EditorApp::prefabFiles()
    {
        return mPrefabFiles;
    }

    const std::vector<std::string>& EditorApp::atlasFrames()
    {
        return mAtlasFrames;
    }

    PendingAction& EditorApp::pendingAction()
    {
        return mPending;
    }

    void EditorApp::applyObjectProperty(ObjectId aUid, const std::string& aProperty, const Value& aValue)
    {
        mView.applyProperty(aUid, aProperty, &aValue);
    }

    void EditorApp::applyCameraProperty(const std::string& aProperty, const Value& aValue)
    {
        if (aProperty == "center")
        {
            mPreviewCamera.center = readVec2(aValue, mPreviewCamera.center);
        }
        else if (aProperty == "zoom")
        {
            mPreviewCamera.zoom = std::max(0.01f, aValue.asFloat(1.0f));
        }
        else if (aProperty == "rotation")
        {
            mPreviewCamera.rotation = aValue.asFloat();
        }
        else if (aProperty == "shake")
        {
            mPreviewCamera.shake = aValue.asFloat();
        }
        else if (aProperty == "shake_frequency")
        {
            mPreviewCamera.shakeFrequency = aValue.asFloat(18.0f);
        }
    }

    void EditorApp::placeObjectOnPath(ObjectId aUid, const Vec2& aWorldPosition, std::optional<float> aWorldRotation)
    {
        const Value position = writeVec2(mView.worldToParentLocal(aUid, aWorldPosition));
        mView.applyProperty(aUid, "position", &position);
        if (aWorldRotation)
        {
            ax::Node* node = mView.findNode(aUid);
            const float parentRotation = node ? mView.worldRotationOf(aUid) - node->getRotation() : 0.0f;
            const Value rotation(*aWorldRotation - parentRotation);
            mView.applyProperty(aUid, "rotation", &rotation);
        }
    }

    const PathSampler* EditorApp::worldPath(ObjectId aPathUid)
    {
        auto cached = mPreviewPaths.find(aPathUid);
        if (cached != mPreviewPaths.end())
        {
            return &cached->second;
        }
        const SceneEntry* entry = mView.find(aPathUid);
        if (!entry || entry->type != "Path")
        {
            return nullptr;
        }
        PathShape shape = readPathShape(entry->props);
        for (auto& point : shape.points)
        {
            const Vec2 world = mView.localToWorld(aPathUid, point.position);
            point.handleIn = mView.localToWorld(aPathUid, point.position + point.handleIn) - world;
            point.handleOut = mView.localToWorld(aPathUid, point.position + point.handleOut) - world;
            point.position = world;
        }
        return &mPreviewPaths.emplace(aPathUid, PathSampler(shape)).first->second;
    }

    void EditorApp::runTimelineEvent(const ValueObject& aAction)
    {
        log(Severity::Info, "Timeline event (preview only): " + writeJsonCompact(Value(aAction)));
    }

    void EditorApp::attachSession(SceneDocument aDocument)
    {
        stopPreview();
        mSession = std::make_unique<EditSession>(std::move(aDocument));
        mSession->addListener([this](const ChangeSet& aChanges)
        {
            onDocumentChanged(aChanges);
        });
        mSelection.clear();
        mHidden.clear();
        mLocked.clear();
        mTimeline = TimelineState{};
        if (!mSession->document().timelines.empty())
        {
            mTimeline.timelineId = mSession->document().timelines.front().id;
        }
        rebuildView();
    }

    void EditorApp::onDocumentChanged(const ChangeSet& aChanges)
    {
        if (aChanges.structure || aChanges.settings || !aChanges.objects.empty())
        {
            rebuildView();
        }
        else
        {
            for (const auto& change : aChanges.properties)
            {
                mView.applyProperty(change.uid, change.property, effectiveProperty(change.uid, change.property));
                if (isHidden(change.uid))
                {
                    if (ax::Node* node = mView.findNode(change.uid))
                    {
                        node->setVisible(false);
                    }
                }
            }
            mPreviewPaths.clear();
        }

        if (aChanges.timelines && !selectedTimeline())
        {
            const SceneDocument* doc = document();
            mTimeline.timelineId = doc && !doc->timelines.empty() ? doc->timelines.front().id : std::string();
            mTimeline.selectedTrack = -1;
            mTimeline.selectedItem = -1;
        }
        pruneSelection();
        if (mTimeline.previewActive)
        {
            refreshPreview();
        }
    }

    void EditorApp::rebuildView()
    {
        if (!mSession)
        {
            return;
        }
        mView.build(mSession->document());
        mView.setParallaxEnabled(mViewport.parallax);
        for (ObjectId uid : mHidden)
        {
            if (ax::Node* node = mView.findNode(uid))
            {
                node->setVisible(false);
            }
        }
        mPreviewPaths.clear();
        applyCamera();
    }

    void EditorApp::pruneSelection()
    {
        const SceneDocument* doc = document();
        std::erase_if(mSelection, [doc](ObjectId aUid)
        {
            return !doc || !doc->findObject(aUid);
        });
    }

    void EditorApp::loadAtlasFrames()
    {
        std::set<std::string> frames;
        static const std::regex keyPattern(R"(<key>([^<]+\.(png|jpg|jpeg|webp))</key>)");
        for (const auto& atlas : mContext.config().preloadAtlases)
        {
            auto text = mContext.content().readText(atlas);
            if (!text)
            {
                continue;
            }
            for (std::sregex_iterator it(text->begin(), text->end(), keyPattern), end; it != end; ++it)
            {
                frames.insert((*it)[1].str());
            }
        }
        mAtlasFrames.assign(frames.begin(), frames.end());
    }

    void EditorApp::drawPlayOverlay()
    {
        if (!mPlayScene)
        {
            return;
        }
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("Play mode", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        World* world = mPlayScene->world();
        if (ImGui::Button("Stop (Esc)"))
        {
            stopPlay();
            ImGui::End();
            return;
        }
        ImGui::SameLine();
        if (ImGui::Button(mPlayScene->isPaused() ? "Resume" : "Pause"))
        {
            mPlayScene->setPaused(!mPlayScene->isPaused());
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!mPlayScene->isPaused());
        if (ImGui::Button("Step"))
        {
            mPlayScene->stepOnce();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Restart") && mPlaySnapshot)
        {
            mPlayScene->loadDocument(*mPlaySnapshot);
            world = mPlayScene->world();
        }

        float timeScale = mPlayScene->timeScale();
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::SliderFloat("Time scale", &timeScale, 0.05f, 3.0f, "%.2fx"))
        {
            mPlayScene->setTimeScale(timeScale);
        }
        if (ImGui::Checkbox("Debug draw", &mPlayDebugDraw) && world)
        {
            world->setDebugDraw(mPlayDebugDraw);
        }
        if (world)
        {
            ImGui::Text("Scene: %s", world->document().id.c_str());
            const ControlMode mode = world->controlMode();
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo("Control mode", std::string(controlModeName(mode)).c_str()))
            {
                for (ControlMode candidate : {ControlMode::None, ControlMode::SideScroll, ControlMode::TopDown})
                {
                    if (ImGui::Selectable(std::string(controlModeName(candidate)).c_str(), candidate == mode))
                    {
                        world->setControlMode(candidate);
                    }
                }
                ImGui::EndCombo();
            }
            if (world->playerUid() != kInvalidObjectId)
            {
                const Vec2 position = world->view().worldPositionOf(world->playerUid());
                ImGui::Text("Player: %.0f, %.0f", position.x, position.y);
            }
            for (const auto& id : world->activeTimelines())
            {
                ImGui::BulletText("timeline %s", id.c_str());
            }
        }
        ImGui::End();
    }
}
