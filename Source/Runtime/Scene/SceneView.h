#pragma once

#include "Core/Base/Diagnostics.h"
#include "Core/Scene/Document.h"
#include "Runtime/Scene/CameraState.h"
#include "Runtime/Scene/NodeFactory.h"

#include "axmol.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace hg
{
    class AppContext;

    struct SceneEntry
    {
        ObjectId uid = kInvalidObjectId;
        ObjectId parent = kInvalidObjectId;
        std::string name;
        std::string type;
        ValueObject props;
        std::vector<ComponentDesc> components;
        ax::Node* node = nullptr;
    };

    class SceneView
    {
    public:
        SceneView(AppContext& aContext, BuildMode aMode);
        ~SceneView();

        SceneView(const SceneView&) = delete;
        SceneView& operator=(const SceneView&) = delete;

        void build(const SceneDocument& aDocument);
        void clear();

        ax::Node* root() const;
        ax::Node* worldLayer() const;
        BuildContext& buildContext();

        const SceneEntry* find(ObjectId aUid) const;
        ax::Node* findNode(ObjectId aUid) const;
        const std::vector<ObjectId>& order() const;
        void forEach(const std::function<void(const SceneEntry&)>& aVisitor) const;

        void applyProperty(ObjectId aUid, const std::string& aProperty, const Value* aValue);
        void applyProperties(ObjectId aUid, const ValueObject& aProps);
        void setSettings(const ValueObject& aSettings);
        ObjectId spawn(const ObjectDesc& aObject, ObjectId aParent);

        void setCamera(const CameraState& aCamera);
        const CameraState& camera() const;
        void setParallaxEnabled(bool aEnabled);

        Vec2 worldSize() const;
        float viewHeight() const;
        Vec2 visibleWorldSize(float aZoom) const;
        float worldScale() const;

        Vec2 worldToScreen(const Vec2& aWorld) const;
        Vec2 screenToWorld(const Vec2& aScreen) const;
        Vec2 worldPositionOf(ObjectId aUid) const;
        Vec2 localToWorld(ObjectId aUid, const Vec2& aLocal) const;
        Vec2 worldToParentLocal(ObjectId aUid, const Vec2& aWorld) const;
        float worldRotationOf(ObjectId aUid) const;

        const Diagnostics& diagnostics() const;

    private:
        void buildObject(const ObjectDesc& aObject, ObjectId aParent, ax::Node* aParentNode);
        void updateBackground();
        void applyParallax();

        AppContext& mContext;
        BuildContext mBuildContext;
        ax::Node* mRoot = nullptr;
        ax::Sprite* mBackground = nullptr;
        ax::Node* mWorldLayer = nullptr;
        std::unordered_map<ObjectId, SceneEntry> mEntries;
        std::vector<ObjectId> mOrder;
        CameraState mCamera;
        Vec2 mWorldSize{3840.0f, 2160.0f};
        float mViewHeight = 2160.0f;
        bool mParallaxEnabled = true;
        ObjectId mNextRuntimeUid = ObjectId(1) << 36;
        Diagnostics mDiagnostics;
    };
}
