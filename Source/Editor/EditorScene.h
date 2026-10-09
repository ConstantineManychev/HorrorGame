#pragma once

#include "axmol.h"

#include <memory>

namespace hg
{
    class AppContext;
    class EditorApp;

    class EditorScene : public ax::Scene
    {
    public:
        static EditorScene* create(AppContext& aContext);

        ~EditorScene() override;

        void onEnter() override;
        void onExit() override;
        void update(float aDelta) override;

    private:
        explicit EditorScene(AppContext& aContext);
        bool initialize();

        AppContext& mContext;
        std::unique_ptr<EditorApp> mApp;
    };
}
