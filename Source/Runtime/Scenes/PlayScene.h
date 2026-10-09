#pragma once

#include "Core/Scene/Document.h"
#include "Runtime/Gameplay/World.h"

#include "axmol.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace hg
{
    class AppContext;

    struct PlayOptions
    {
        std::function<std::optional<SceneDocument>(const std::string&)> loadScene;
        std::function<void()> onExitRequested;
        std::function<void(World&)> onWorldStarted;
    };

    class PlayScene : public ax::Scene
    {
    public:
        static PlayScene* create(AppContext& aContext, PlayOptions aOptions = {});

        bool loadScene(const std::string& aSceneId, std::optional<Vec2> aSpawnOverride = std::nullopt);
        void loadDocument(SceneDocument aDocument, std::optional<Vec2> aSpawnOverride = std::nullopt);
        World* world() const;

        void setPaused(bool aPaused);
        bool isPaused() const;
        void stepOnce();
        void setTimeScale(float aScale);
        float timeScale() const;

        void update(float aDelta) override;
        void onEnter() override;
        void onExit() override;

    private:
        PlayScene(AppContext& aContext, PlayOptions aOptions);

        AppContext& mContext;
        PlayOptions mOptions;
        std::unique_ptr<World> mWorld;
        std::optional<std::string> mPendingScene;
        bool mPendingQuit = false;
        bool mPaused = false;
        bool mStepRequested = false;
        float mTimeScale = 1.0f;
    };
}
