#include "Runtime/Gameplay/World.h"

#include "Core/Animation/Path.h"
#include "Core/Base/ValueConvert.h"
#include "Runtime/App/AppContext.h"
#include "Runtime/App/Log.h"
#include "Runtime/Scene/AxConvert.h"

#include <cmath>

namespace hg
{
    namespace
    {
        constexpr int kDebugZ = 1000000;
        constexpr int kSortRange = 50000;
        constexpr int kSortLayerStride = 100000;

        void drawRectOutline(ax::DrawNode* aNode, const Rect& aRect, const ax::Color4F& aColor)
        {
            aNode->drawRect(toAx(aRect.origin), toAx(aRect.origin + aRect.size), aColor, 3.0f);
        }

        float pingPong(float aValue)
        {
            const float wrapped = std::fmod(aValue, 2.0f);
            return wrapped <= 1.0f ? wrapped : 2.0f - wrapped;
        }
    }

    World::World(AppContext& aContext, SceneDocument aDocument, Callbacks aCallbacks)
        : mContext(aContext)
        , mDocument(std::move(aDocument))
        , mCallbacks(std::move(aCallbacks))
        , mView(aContext, BuildMode::Play)
    {
        mView.buildContext().runActions = [this](const Value& aActions)
        {
            runActions(aActions);
        };
        mView.build(mDocument);

        mMode = parseControlMode(mDocument.settings.get("control_mode").asString("none")).value_or(ControlMode::None);

        CameraRigSettings settings;
        settings.worldSize = mView.worldSize();
        settings.deadZone = readVec2(mDocument.settings.get("camera_dead_zone"), settings.deadZone);
        settings.damping = mDocument.settings.get("camera_damping").asFloat(settings.damping);
        mCamera.configure(settings);
        mCamera.setVisibleSizeProvider([this](float aZoom)
        {
            return mView.visibleWorldSize(aZoom);
        });

        mDebugNode = ax::DrawNode::create();
        mView.worldLayer()->addChild(mDebugNode, kDebugZ);
    }

    World::~World()
    {
        mTimelines.clear();
    }

    ax::Node* World::root() const
    {
        return mView.root();
    }

    const SceneDocument& World::document() const
    {
        return mDocument;
    }

    SceneView& World::view()
    {
        return mView;
    }

    CameraRig& World::camera()
    {
        return mCamera;
    }

    void World::start(std::optional<Vec2> aSpawnOverride)
    {
        collectStaticGeometry();
        spawnFromPoints(aSpawnOverride);

        if (mPlayer)
        {
            mCamera.setTarget(mPlayer->body.boundsAt(mPlayer->state.position).center());
            mCamera.reset(mPlayer->state.position);
            mCamera.snapToTarget();
        }
        else
        {
            mCamera.reset(mView.worldSize() * 0.5f);
        }

        const std::string music = mDocument.settings.get("music").asString();
        if (!music.empty())
        {
            mContext.audio().playMusic(music, true);
        }
        runActions(mDocument.settings.get("on_start"));
        updateCamera(0.0f);
        flushDeferred();
    }

    void World::update(float aDelta)
    {
        mUpdating = true;
        mCamera.beginFrame();
        updatePlayer(aDelta);
        updateFollowers(aDelta);
        updateTimelines(aDelta);
        updateTriggers();
        updateSorting();
        updateCamera(aDelta);
        drawDebug();
        mUpdating = false;
        flushDeferred();
    }

    void World::runActions(const Value& aActions)
    {
        for (const auto& action : aActions.asArray())
        {
            if (const ValueObject* object = action.getObject())
            {
                runAction(*object);
            }
        }
    }

    void World::runAction(const ValueObject& aAction)
    {
        const std::string name = aAction.get("do").asString();
        if (name == "change_scene")
        {
            const std::string scene = aAction.get("scene").asString();
            mDeferred.push_back([this, scene]()
            {
                if (mCallbacks.changeScene)
                {
                    mCallbacks.changeScene(scene);
                }
            });
        }
        else if (name == "play_timeline")
        {
            const std::string id = aAction.get("timeline").asString();
            if (mUpdating)
            {
                mDeferred.push_back([this, id]()
                {
                    playTimeline(id);
                });
            }
            else
            {
                playTimeline(id);
            }
        }
        else if (name == "stop_timeline")
        {
            const std::string id = aAction.get("timeline").asString();
            mDeferred.push_back([this, id]()
            {
                stopTimeline(id);
            });
        }
        else if (name == "set_control_mode")
        {
            if (auto mode = parseControlMode(aAction.get("mode").asString()))
            {
                setControlMode(*mode);
            }
        }
        else if (name == "set_visible")
        {
            const ObjectId target = static_cast<ObjectId>(std::max<int64_t>(0, aAction.get("target").asInt()));
            applyObjectProperty(target, "visible", Value(aAction.get("visible").asBool(true)));
        }
        else if (name == "play_sound")
        {
            mContext.audio().playSound(aAction.get("file").asString());
        }
        else if (name == "play_music")
        {
            mContext.audio().playMusic(aAction.get("file").asString(), aAction.get("loop").asBool(true));
        }
        else if (name == "stop_music")
        {
            mContext.audio().stopMusic();
        }
        else if (name == "set_input_enabled")
        {
            setInputEnabled(aAction.get("enabled").asBool(true));
        }
        else if (name == "quit_game")
        {
            mDeferred.push_back([this]()
            {
                if (mCallbacks.quit)
                {
                    mCallbacks.quit();
                }
            });
        }
        else
        {
            Log::warning("Unknown action '" + name + "' in scene " + mDocument.id);
        }
    }

    void World::setControlMode(ControlMode aMode)
    {
        if (aMode == mMode)
        {
            return;
        }
        if (mMode == ControlMode::TopDown)
        {
            mView.forEach([this](const SceneEntry& aEntry)
            {
                if (aEntry.props.get("y_sort").asBool(false))
                {
                    mContext.nodes().applyProperty(aEntry.node, aEntry.type, "z", aEntry.props.find("z"), mView.buildContext());
                }
            });
        }
        mMode = aMode;
        if (mPlayer)
        {
            mPlayer->state.velocity = {};
            mPlayer->state.grounded = false;
        }
        Log::info("Control mode: " + std::string(controlModeName(aMode)));
    }

    ControlMode World::controlMode() const
    {
        return mMode;
    }

    void World::setInputEnabled(bool aEnabled)
    {
        mInputEnabled = aEnabled;
    }

    ObjectId World::playerUid() const
    {
        return mPlayer ? mPlayer->uid : kInvalidObjectId;
    }

    TimelinePlayer* World::playTimeline(const std::string& aId)
    {
        const TimelineDesc* timeline = mDocument.findTimeline(aId);
        if (!timeline)
        {
            Log::warning("Timeline '" + aId + "' not found in scene " + mDocument.id);
            return nullptr;
        }
        auto player = std::make_unique<TimelinePlayer>(*timeline, *this);
        TimelinePlayer* raw = player.get();
        mTimelines[aId] = std::move(player);
        raw->play(0.0f);
        return raw;
    }

    void World::stopTimeline(const std::string& aId)
    {
        mTimelines.erase(aId);
    }

    std::vector<std::string> World::activeTimelines() const
    {
        std::vector<std::string> ids;
        for (const auto& [id, player] : mTimelines)
        {
            ids.push_back(id);
        }
        return ids;
    }

    void World::setDebugDraw(bool aEnabled)
    {
        mDebugDraw = aEnabled;
        if (!aEnabled && mDebugNode)
        {
            mDebugNode->clear();
        }
    }

    bool World::debugDraw() const
    {
        return mDebugDraw;
    }

    void World::applyObjectProperty(ObjectId aUid, const std::string& aProperty, const Value& aValue)
    {
        mView.applyProperty(aUid, aProperty, &aValue);
        if (mPlayer && aUid == mPlayer->uid && aProperty == "position")
        {
            mPlayer->state.position = readVec2(aValue, mPlayer->state.position);
            mPlayer->state.velocity = {};
        }
    }

    void World::applyCameraProperty(const std::string& aProperty, const Value& aValue)
    {
        mCamera.drive(aProperty, aValue);
    }

    void World::placeObjectOnPath(ObjectId aUid, const Vec2& aWorldPosition, std::optional<float> aWorldRotation)
    {
        const Vec2 local = mView.worldToParentLocal(aUid, aWorldPosition);
        applyObjectProperty(aUid, "position", writeVec2(local));
        if (aWorldRotation)
        {
            ax::Node* node = mView.findNode(aUid);
            const float parentRotation = node ? mView.worldRotationOf(aUid) - node->getRotation() : 0.0f;
            applyObjectProperty(aUid, "rotation", Value(*aWorldRotation - parentRotation));
        }
    }

    const PathSampler* World::worldPath(ObjectId aPathUid)
    {
        auto cached = mPaths.find(aPathUid);
        if (cached != mPaths.end())
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
        return &mPaths.emplace(aPathUid, PathSampler(shape)).first->second;
    }

    void World::runTimelineEvent(const ValueObject& aAction)
    {
        runAction(aAction);
    }

    void World::collectStaticGeometry()
    {
        mSolids.clear();
        mTriggers.clear();
        mFollowers.clear();
        mView.forEach([this](const SceneEntry& aEntry)
        {
            if (aEntry.type == "Collider")
            {
                mSolids.push_back({areaRect(aEntry.uid), aEntry.props.get("one_way").asBool(false)});
            }
            else if (aEntry.type == "Trigger")
            {
                mTriggers.push_back({aEntry.uid});
            }
            for (const auto& component : aEntry.components)
            {
                if (component.type != "PathFollower")
                {
                    continue;
                }
                PathFollower follower;
                follower.uid = aEntry.uid;
                follower.path = static_cast<ObjectId>(std::max<int64_t>(0, component.props.get("path").asInt()));
                follower.duration = std::max(0.01f, component.props.get("duration").asFloat(4.0f));
                follower.mode = component.props.get("mode").asString("loop");
                follower.ease = Easing::of(parseEaseType(component.props.get("ease").asString("linear")).value_or(EaseType::Linear));
                follower.orient = component.props.get("orient").asBool(false);
                follower.running = component.props.get("autostart").asBool(true);
                mFollowers.push_back(follower);
            }
        });
    }

    void World::spawnFromPoints(std::optional<Vec2> aSpawnOverride)
    {
        std::vector<const SceneEntry*> points;
        mView.forEach([&points](const SceneEntry& aEntry)
        {
            if (aEntry.type == "SpawnPoint")
            {
                points.push_back(&aEntry);
            }
        });

        for (const SceneEntry* point : points)
        {
            const bool isPlayer = point->props.get("player").asBool(true) && !mPlayer;
            ObjectDesc instance;
            instance.name = isPlayer ? "Player" : point->name + "_spawn";
            instance.prefab = point->props.get("prefab").asString();
            if (instance.prefab.empty())
            {
                continue;
            }
            Vec2 position = mView.worldPositionOf(point->uid);
            if (isPlayer && aSpawnOverride)
            {
                position = *aSpawnOverride;
            }
            instance.props.set("position", writeVec2(position));

            const ObjectId uid = mView.spawn(instance, kInvalidObjectId);
            const SceneEntry* spawned = mView.find(uid);
            if (!isPlayer || !spawned)
            {
                continue;
            }

            Character character;
            character.uid = uid;
            for (const auto& component : spawned->components)
            {
                if (component.type == "CharacterBody")
                {
                    character.body = MotorBody::fromProps(component.props);
                }
                else if (component.type == "PlayerController")
                {
                    character.params = MotorParams::fromProps(component.props);
                }
            }
            character.state.position = position;
            mPlayer = character;
        }
    }

    void World::updatePlayer(float aDelta)
    {
        if (!mPlayer || mMode == ControlMode::None)
        {
            return;
        }
        InputService& input = mContext.input();
        MotorInput motorInput;
        if (mInputEnabled)
        {
            motorInput.move = input.moveAxis();
            motorInput.jumpPressed = input.consumePressed(InputActions::kJump);
            motorInput.jumpHeld = input.isHeld(InputActions::kJump);
        }

        const Rect worldBounds{{0.0f, 0.0f}, mView.worldSize()};
        stepMotor(mPlayer->state, mPlayer->params, mPlayer->body, motorInput, mMode, mSolids, worldBounds, aDelta);
        const Value position = writeVec2(mPlayer->state.position);
        mView.applyProperty(mPlayer->uid, "position", &position);
        if (ax::Node* node = mView.findNode(mPlayer->uid))
        {
            const float scaleX = std::abs(node->getScaleX());
            node->setScaleX(mMode == ControlMode::SideScroll ? scaleX * static_cast<float>(mPlayer->state.facing) : scaleX);
        }
    }

    void World::updateTriggers()
    {
        if (!mPlayer)
        {
            return;
        }
        const Rect body = mPlayer->body.boundsAt(mPlayer->state.position);
        for (auto& trigger : mTriggers)
        {
            const SceneEntry* entry = mView.find(trigger.uid);
            if (!entry)
            {
                continue;
            }
            const bool inside = areaRect(trigger.uid).intersects(body);
            if (inside && !trigger.inside)
            {
                const bool once = entry->props.get("once").asBool(true);
                if (!once || !trigger.fired)
                {
                    trigger.fired = true;
                    runActions(entry->props.get("on_enter"));
                }
            }
            else if (!inside && trigger.inside)
            {
                runActions(entry->props.get("on_exit"));
            }
            trigger.inside = inside;
        }
    }

    void World::updateFollowers(float aDelta)
    {
        for (auto& follower : mFollowers)
        {
            if (!follower.running)
            {
                continue;
            }
            const PathSampler* sampler = worldPath(follower.path);
            if (!sampler || sampler->empty())
            {
                continue;
            }
            follower.time += aDelta;
            float progress = follower.time / follower.duration;
            if (follower.mode == "once")
            {
                if (progress >= 1.0f)
                {
                    progress = 1.0f;
                    follower.running = false;
                }
            }
            else if (follower.mode == "ping_pong")
            {
                progress = pingPong(progress);
            }
            else
            {
                progress = std::fmod(progress, 1.0f);
            }
            const PathSample sample = sampler->sampleNormalized(follower.ease.apply(progress));
            std::optional<float> rotation;
            if (follower.orient)
            {
                rotation = -radiansToDegrees(sample.tangent.angle());
            }
            placeObjectOnPath(follower.uid, sample.position, rotation);
        }
    }

    void World::updateTimelines(float aDelta)
    {
        for (auto& [id, player] : mTimelines)
        {
            player->update(aDelta);
        }
        for (auto it = mTimelines.begin(); it != mTimelines.end();)
        {
            if (it->second->isFinished())
            {
                it = mTimelines.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void World::updateSorting()
    {
        if (mMode != ControlMode::TopDown)
        {
            return;
        }
        mView.forEach([](const SceneEntry& aEntry)
        {
            if (!aEntry.props.get("y_sort").asBool(false))
            {
                return;
            }
            const int baseZ = static_cast<int>(aEntry.props.get("z").asInt(0));
            const int depth = std::clamp(static_cast<int>(aEntry.node->getPositionY()), -kSortRange, kSortRange);
            aEntry.node->setLocalZOrder(baseZ * kSortLayerStride + (kSortRange - depth));
        });
    }

    void World::updateCamera(float aDelta)
    {
        if (mPlayer)
        {
            mCamera.setTarget(mPlayer->body.boundsAt(mPlayer->state.position).center());
        }
        mCamera.update(aDelta);
        mView.setCamera(mCamera.output());
    }

    void World::drawDebug()
    {
        if (!mDebugDraw || !mDebugNode)
        {
            return;
        }
        mDebugNode->clear();
        for (const auto& solid : mSolids)
        {
            drawRectOutline(mDebugNode, solid.rect, solid.oneWay ? ax::Color4F(1.0f, 0.6f, 0.1f, 1.0f) : ax::Color4F(1.0f, 0.2f, 0.2f, 1.0f));
        }
        for (const auto& trigger : mTriggers)
        {
            drawRectOutline(mDebugNode, areaRect(trigger.uid), trigger.inside ? ax::Color4F(1.0f, 1.0f, 0.3f, 1.0f) : ax::Color4F(0.8f, 0.8f, 0.2f, 0.6f));
        }
        for (const auto& [uid, sampler] : mPaths)
        {
            const auto& points = sampler.polyline();
            for (size_t index = 1; index < points.size(); ++index)
            {
                mDebugNode->drawLine(toAx(points[index - 1]), toAx(points[index]), ax::Color4F(0.2f, 0.9f, 1.0f, 1.0f), 2.0f);
            }
        }
        if (mPlayer)
        {
            drawRectOutline(mDebugNode, mPlayer->body.boundsAt(mPlayer->state.position), ax::Color4F(0.2f, 1.0f, 0.3f, 1.0f));
        }
    }

    Rect World::areaRect(ObjectId aUid) const
    {
        const SceneEntry* entry = mView.find(aUid);
        if (!entry)
        {
            return {};
        }
        const Vec2 size = readVec2(entry->props.get("size"), {});
        return Rect::fromCenter(mView.worldPositionOf(aUid), size);
    }

    void World::flushDeferred()
    {
        while (!mDeferred.empty())
        {
            auto pending = std::move(mDeferred);
            mDeferred.clear();
            for (auto& action : pending)
            {
                action();
            }
        }
    }
}
