#include "Editor/EditorScene.h"

#include "Editor/EditorApp.h"
#include "Runtime/App/AppContext.h"

#include "ImGui/ImGuiPresenter.h"

namespace hg
{
    namespace
    {
        constexpr const char* kEditorLoopId = "#edit";
        constexpr float kEditorFontSize = 16.0f;
    }

    EditorScene* EditorScene::create(AppContext& aContext)
    {
        auto* scene = new EditorScene(aContext);
        if (scene->initialize())
        {
            scene->autorelease();
            return scene;
        }
        delete scene;
        return nullptr;
    }

    EditorScene::EditorScene(AppContext& aContext)
        : mContext(aContext)
    {
    }

    EditorScene::~EditorScene()
    {
        mApp.reset();
    }

    bool EditorScene::initialize()
    {
        if (!ax::Scene::init())
        {
            return false;
        }
        mContext.input().setTouchControlsEnabled(false);
        ax::extension::ImGuiPresenter::getInstance();
        mApp = std::make_unique<EditorApp>(mContext, *this);
        return true;
    }

    void EditorScene::onEnter()
    {
        ax::Scene::onEnter();
        auto* presenter = ax::extension::ImGuiPresenter::getInstance();
        const std::string font = mContext.content().fullPath("fonts/arial.ttf");
        if (!font.empty())
        {
            presenter->addFont(font, kEditorFontSize);
        }
        presenter->addRenderLoop(kEditorLoopId, [this]()
        {
            mApp->drawGui();
        }, this);
        scheduleUpdate();
    }

    void EditorScene::onExit()
    {
        ax::extension::ImGuiPresenter::getInstance()->removeRenderLoop(kEditorLoopId);
        unscheduleUpdate();
        ax::Scene::onExit();
    }

    void EditorScene::update(float aDelta)
    {
        mApp->update(aDelta);
    }
}
