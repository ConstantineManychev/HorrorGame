#pragma once

#include "Core/Gameplay/CharacterMotor.h"
#include "Core/Scene/Document.h"
#include "Runtime/Animation/TimelinePlayer.h"
#include "Runtime/Gameplay/CameraRig.h"
#include "Runtime/Scene/SceneView.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace hg
{
    class AppContext;

    class World : public TimelineTarget
    {
    public:
        struct Callbacks
        {
            std::function<void(const std::string&)> changeScene;
            std::function<void()> quit;
        };

        World(AppContext& aContext, SceneDocument aDocument, Callbacks aCallbacks);
        ~World() override;

        World(const World&) = delete;
        World& operator=(const World&) = delete;

        ax::Node* root() const;
        const SceneDocument& document() const;
        SceneView& view();
        CameraRig& camera();

        void start(std::optional<Vec2> aSpawnOverride = std::nullopt);
        void update(float aDelta);

        void runActions(const Value& aActions);
        void runAction(const ValueObject& aAction);

        void setControlMode(ControlMode aMode);
        ControlMode controlMode() const;
        void setInputEnabled(bool aEnabled);
        ObjectId playerUid() const;

        TimelinePlayer* playTimeline(const std::string& aId);
        void stopTimeline(const std::string& aId);
        std::vector<std::string> activeTimelines() const;

        void setDebugDraw(bool aEnabled);
        bool debugDraw() const;

        void applyObjectProperty(ObjectId aUid, const std::string& aProperty, const Value& aValue) override;
        void applyCameraProperty(const std::string& aProperty, const Value& aValue) override;
        void placeObjectOnPath(ObjectId aUid, const Vec2& aWorldPosition, std::optional<float> aWorldRotation) override;
        const PathSampler* worldPath(ObjectId aPathUid) override;
        void runTimelineEvent(const ValueObject& aAction) override;

    private:
        struct Character
        {
            ObjectId uid = kInvalidObjectId;
            MotorParams params;
            MotorBody body;
            MotorState state;
        };

        struct TriggerArea
        {
            ObjectId uid = kInvalidObjectId;
            bool inside = false;
            bool fired = false;
        };

        struct PathFollower
        {
            ObjectId uid = kInvalidObjectId;
            ObjectId path = kInvalidObjectId;
            float duration = 4.0f;
            std::string mode;
            Easing ease;
            bool orient = false;
            bool running = true;
            float time = 0.0f;
        };

        void collectStaticGeometry();
        void spawnFromPoints(std::optional<Vec2> aSpawnOverride);
        void updatePlayer(float aDelta);
        void updateTriggers();
        void updateFollowers(float aDelta);
        void updateTimelines(float aDelta);
        void updateSorting();
        void updateCamera(float aDelta);
        void drawDebug();
        Rect areaRect(ObjectId aUid) const;
        void flushDeferred();

        AppContext& mContext;
        SceneDocument mDocument;
        Callbacks mCallbacks;
        SceneView mView;
        CameraRig mCamera;
        ControlMode mMode = ControlMode::None;
        bool mInputEnabled = true;
        bool mDebugDraw = false;
        ax::DrawNode* mDebugNode = nullptr;

        std::vector<SolidBox> mSolids;
        std::vector<TriggerArea> mTriggers;
        std::vector<PathFollower> mFollowers;
        std::optional<Character> mPlayer;
        std::unordered_map<ObjectId, PathSampler> mPaths;
        std::map<std::string, std::unique_ptr<TimelinePlayer>> mTimelines;
        std::vector<std::function<void()>> mDeferred;
        bool mUpdating = false;
    };
}
