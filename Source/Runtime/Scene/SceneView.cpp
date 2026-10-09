#include "Runtime/Scene/SceneView.h"

#include "Core/Base/ValueConvert.h"
#include "Core/Scene/Prefab.h"
#include "Runtime/App/AppContext.h"
#include "Runtime/App/Log.h"
#include "Runtime/Scene/AxConvert.h"

namespace hg
{
    namespace
    {
        constexpr int kBackgroundZ = -1000000;
    }

    SceneView::SceneView(AppContext& aContext, BuildMode aMode)
        : mContext(aContext)
    {
        mBuildContext.mode = aMode;
        mBuildContext.content = &aContext.content();

        mRoot = ax::Node::create();
        mRoot->retain();

        mBackground = ax::Sprite::create();
        mBackground->setTexture(nullptr);
        mBackground->setAnchorPoint(ax::Vec2::ZERO);
        mBackground->setColor(ax::Color3B::BLACK);
        mRoot->addChild(mBackground, kBackgroundZ);

        mWorldLayer = ax::Node::create();
        mWorldLayer->setCascadeOpacityEnabled(true);
        mRoot->addChild(mWorldLayer);
        updateBackground();
    }

    SceneView::~SceneView()
    {
        clear();
        if (mRoot)
        {
            mRoot->removeFromParent();
            mRoot->release();
            mRoot = nullptr;
        }
    }

    void SceneView::build(const SceneDocument& aDocument)
    {
        clear();
        mDiagnostics.clear();
        mDiagnostics.setSource(aDocument.id);
        setSettings(aDocument.settings);

        const std::vector<ObjectDesc> expanded = expandPrefabs(aDocument.objects, mContext.content().prefabs(), mDiagnostics);
        for (const auto& object : expanded)
        {
            buildObject(object, kInvalidObjectId, mWorldLayer);
        }
        if (!mDiagnostics.empty())
        {
            Log::diagnostics(mDiagnostics);
        }
        setCamera(mCamera);
    }

    void SceneView::clear()
    {
        if (mWorldLayer)
        {
            mWorldLayer->removeAllChildren();
        }
        mEntries.clear();
        mOrder.clear();
    }

    ax::Node* SceneView::root() const
    {
        return mRoot;
    }

    ax::Node* SceneView::worldLayer() const
    {
        return mWorldLayer;
    }

    BuildContext& SceneView::buildContext()
    {
        return mBuildContext;
    }

    const SceneEntry* SceneView::find(ObjectId aUid) const
    {
        auto it = mEntries.find(aUid);
        return it != mEntries.end() ? &it->second : nullptr;
    }

    ax::Node* SceneView::findNode(ObjectId aUid) const
    {
        const SceneEntry* entry = find(aUid);
        return entry ? entry->node : nullptr;
    }

    const std::vector<ObjectId>& SceneView::order() const
    {
        return mOrder;
    }

    void SceneView::forEach(const std::function<void(const SceneEntry&)>& aVisitor) const
    {
        for (ObjectId uid : mOrder)
        {
            if (const SceneEntry* entry = find(uid))
            {
                aVisitor(*entry);
            }
        }
    }

    void SceneView::applyProperty(ObjectId aUid, const std::string& aProperty, const Value* aValue)
    {
        auto it = mEntries.find(aUid);
        if (it == mEntries.end())
        {
            return;
        }
        SceneEntry& entry = it->second;
        if (aValue)
        {
            entry.props.set(aProperty, *aValue);
        }
        else
        {
            entry.props.erase(aProperty);
        }
        mContext.nodes().applyProperty(entry.node, entry.type, aProperty, aValue, mBuildContext);
        if (entry.type == "ParallaxLayer")
        {
            applyParallax();
        }
    }

    void SceneView::applyProperties(ObjectId aUid, const ValueObject& aProps)
    {
        for (const auto& member : aProps)
        {
            applyProperty(aUid, member.key, &member.value);
        }
    }

    void SceneView::setSettings(const ValueObject& aSettings)
    {
        mWorldSize = readVec2(aSettings.get("world_size"), {3840.0f, 2160.0f});
        mViewHeight = std::max(1.0f, aSettings.get("view_height").asFloat(mWorldSize.y));
        mBackground->setColor(toAx3B(readColor(aSettings.get("background"), Color{0, 0, 0, 255})));
        setCamera(mCamera);
    }

    ObjectId SceneView::spawn(const ObjectDesc& aObject, ObjectId aParent)
    {
        ObjectDesc instance = aObject;
        if (instance.uid == kInvalidObjectId || mEntries.count(instance.uid))
        {
            instance.uid = mNextRuntimeUid++;
        }
        ObjectDesc object = expandPrefabs(instance, mContext.content().prefabs(), mDiagnostics);
        ax::Node* parentNode = aParent == kInvalidObjectId ? mWorldLayer : findNode(aParent);
        if (!parentNode)
        {
            parentNode = mWorldLayer;
            aParent = kInvalidObjectId;
        }
        buildObject(object, aParent, parentNode);
        return object.uid;
    }

    void SceneView::setCamera(const CameraState& aCamera)
    {
        mCamera = aCamera;
        updateBackground();

        const ax::Vec2 visibleOrigin = ax::Director::getInstance()->getVisibleOrigin();
        const ax::Size visibleSize = ax::Director::getInstance()->getVisibleSize();
        const ax::Vec2 screenCenter = visibleOrigin + ax::Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.5f);

        mWorldLayer->setPosition(ax::Vec2::ZERO);
        mWorldLayer->setScale(worldScale());
        mWorldLayer->setRotation(mCamera.rotation);
        const ax::Vec2 focus = mWorldLayer->convertToWorldSpace(toAx(mCamera.center + mCamera.shakeOffset));
        mWorldLayer->setPosition(screenCenter - focus);
        applyParallax();
    }

    const CameraState& SceneView::camera() const
    {
        return mCamera;
    }

    void SceneView::setParallaxEnabled(bool aEnabled)
    {
        mParallaxEnabled = aEnabled;
        applyParallax();
    }

    Vec2 SceneView::worldSize() const
    {
        return mWorldSize;
    }

    float SceneView::viewHeight() const
    {
        return mViewHeight;
    }

    Vec2 SceneView::visibleWorldSize(float aZoom) const
    {
        const ax::Size visibleSize = ax::Director::getInstance()->getVisibleSize();
        const float scale = visibleSize.height / mViewHeight * std::max(0.01f, aZoom);
        return {visibleSize.width / scale, visibleSize.height / scale};
    }

    float SceneView::worldScale() const
    {
        const ax::Size visibleSize = ax::Director::getInstance()->getVisibleSize();
        return visibleSize.height / mViewHeight * std::max(0.01f, mCamera.zoom);
    }

    Vec2 SceneView::worldToScreen(const Vec2& aWorld) const
    {
        return fromAx(mWorldLayer->convertToWorldSpace(toAx(aWorld)));
    }

    Vec2 SceneView::screenToWorld(const Vec2& aScreen) const
    {
        return fromAx(mWorldLayer->convertToNodeSpace(toAx(aScreen)));
    }

    Vec2 SceneView::worldPositionOf(ObjectId aUid) const
    {
        ax::Node* node = findNode(aUid);
        if (!node || !node->getParent())
        {
            return {};
        }
        return screenToWorld(fromAx(node->getParent()->convertToWorldSpace(node->getPosition())));
    }

    Vec2 SceneView::localToWorld(ObjectId aUid, const Vec2& aLocal) const
    {
        ax::Node* node = findNode(aUid);
        if (!node)
        {
            return aLocal;
        }
        return screenToWorld(fromAx(node->convertToWorldSpace(toAx(aLocal))));
    }

    Vec2 SceneView::worldToParentLocal(ObjectId aUid, const Vec2& aWorld) const
    {
        ax::Node* node = findNode(aUid);
        if (!node || !node->getParent())
        {
            return aWorld;
        }
        return fromAx(node->getParent()->convertToNodeSpace(toAx(worldToScreen(aWorld))));
    }

    float SceneView::worldRotationOf(ObjectId aUid) const
    {
        float rotation = 0.0f;
        for (ax::Node* node = findNode(aUid); node && node != mWorldLayer; node = node->getParent())
        {
            rotation += node->getRotation();
        }
        return rotation;
    }

    const Diagnostics& SceneView::diagnostics() const
    {
        return mDiagnostics;
    }

    void SceneView::buildObject(const ObjectDesc& aObject, ObjectId aParent, ax::Node* aParentNode)
    {
        NodeFactory& nodes = mContext.nodes();
        ax::Node* node = nodes.create(aObject.type, mBuildContext);
        node->setName(aObject.name);
        nodes.applyAll(node, aObject.type, aObject.props, mBuildContext);
        aParentNode->addChild(node);

        SceneEntry entry;
        entry.uid = aObject.uid;
        entry.parent = aParent;
        entry.name = aObject.name;
        entry.type = aObject.type;
        entry.props = aObject.props;
        entry.components = aObject.components;
        entry.node = node;
        mEntries[aObject.uid] = std::move(entry);
        mOrder.push_back(aObject.uid);

        for (const auto& child : aObject.children)
        {
            buildObject(child, aObject.uid, node);
        }
    }

    void SceneView::updateBackground()
    {
        const ax::Vec2 origin = ax::Director::getInstance()->getVisibleOrigin();
        const ax::Size size = ax::Director::getInstance()->getVisibleSize();
        mBackground->setPosition(origin);
        mBackground->setTextureRect(ax::Rect(0.0f, 0.0f, size.width, size.height));
    }

    void SceneView::applyParallax()
    {
        for (auto& [uid, entry] : mEntries)
        {
            if (entry.type != "ParallaxLayer")
            {
                continue;
            }
            const Vec2 base = readVec2(entry.props.get("position"), {});
            const Vec2 factor = readVec2(entry.props.get("factor"), {1.0f, 1.0f});
            Vec2 offset;
            if (mParallaxEnabled)
            {
                offset = {mCamera.center.x * (1.0f - factor.x), mCamera.center.y * (1.0f - factor.y)};
            }
            entry.node->setPosition(toAx(base + offset));
        }
    }
}
