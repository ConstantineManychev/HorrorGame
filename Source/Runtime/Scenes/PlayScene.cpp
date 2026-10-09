#include "Runtime/Scenes/PlayScene.h"

#include "Runtime/App/AppContext.h"
#include "Runtime/App/Log.h"

namespace hg
{
    namespace
    {
        constexpr float kMaxFrameDelta = 1.0f / 20.0f;
    }

    PlayScene* PlayScene::create(AppContext& aContext, PlayOptions aOptions)
    {
        auto* scene = new PlayScene(aContext, std::move(aOptions));
        if (scene->init())
        {
            scene->autorelease();
            return scene;
        }
        delete scene;
        return nullptr;
    }

    PlayScene::PlayScene(AppContext& aContext, PlayOptions aOptions)
        : mContext(aContext)
        , mOptions(std::move(aOptions))
    {
        if (!mOptions.loadScene)
        {
            mOptions.loadScene = [&aContext](const std::string& aSceneId) -> std::optional<SceneDocument>
            {
                Diagnostics diagnostics;
                auto document = aContext.content().loadScene(aSceneId, diagnostics);
                Log::diagnostics(diagnostics);
                return document;
            };
        }
    }

    bool PlayScene::loadScene(const std::string& aSceneId, std::optional<Vec2> aSpawnOverride)
    {
        auto document = mOptions.loadScene(aSceneId);
        if (!document)
        {
            Log::error("Cannot load scene '" + aSceneId + "'");
            return false;
        }
        loadDocument(std::move(*document), aSpawnOverride);
        return true;
    }

    void PlayScene::loadDocument(SceneDocument aDocument, std::optional<Vec2> aSpawnOverride)
    {
        if (mWorld)
        {
            mWorld->root()->removeFromParent();
            mWorld.reset();
        }
        mContext.input().reset();

        World::Callbacks callbacks;
        callbacks.changeScene = [this](const std::string& aSceneId)
        {
            mPendingScene = aSceneId;
        };
        callbacks.quit = [this]()
        {
            mPendingQuit = true;
        };

        Log::info("Loading scene '" + aDocument.id + "'");
        mWorld = std::make_unique<World>(mContext, std::move(aDocument), std::move(callbacks));
        addChild(mWorld->root());
        mWorld->start(aSpawnOverride);
        if (mOptions.onWorldStarted)
        {
            mOptions.onWorldStarted(*mWorld);
        }
    }

    World* PlayScene::world() const
    {
        return mWorld.get();
    }

    void PlayScene::setPaused(bool aPaused)
    {
        mPaused = aPaused;
    }

    bool PlayScene::isPaused() const
    {
        return mPaused;
    }

    void PlayScene::stepOnce()
    {
        mStepRequested = true;
    }

    void PlayScene::setTimeScale(float aScale)
    {
        mTimeScale = std::max(0.0f, aScale);
    }

    float PlayScene::timeScale() const
    {
        return mTimeScale;
    }

    void PlayScene::update(float aDelta)
    {
        if (mContext.input().consumePressed(InputActions::kPause) && mOptions.onExitRequested)
        {
            mOptions.onExitRequested();
            return;
        }

        if (mWorld && (!mPaused || mStepRequested))
        {
            const float delta = mStepRequested ? 1.0f / 60.0f : std::min(aDelta, kMaxFrameDelta) * mTimeScale;
            mStepRequested = false;
            mWorld->update(delta);
        }

        if (mPendingQuit)
        {
            mPendingQuit = false;
            if (mOptions.onExitRequested)
            {
                mOptions.onExitRequested();
            }
            else
            {
                ax::Director::getInstance()->end();
            }
            return;
        }

        if (mPendingScene)
        {
            const std::string sceneId = *mPendingScene;
            mPendingScene.reset();
            loadScene(sceneId);
        }
    }

    void PlayScene::onEnter()
    {
        ax::Scene::onEnter();
        scheduleUpdate();
    }

    void PlayScene::onExit()
    {
        unscheduleUpdate();
        ax::Scene::onExit();
    }
}
