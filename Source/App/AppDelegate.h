#pragma once

#include "axmol.h"

#include <memory>

namespace hg
{
    class AppContext;
}

class AppDelegate : private ax::Application
{
public:
    AppDelegate();
    ~AppDelegate() override;

    void initGfxContextAttrs() override;
    bool applicationDidFinishLaunching() override;
    void applicationDidEnterBackground() override;
    void applicationWillEnterForeground() override;
    void applicationWillQuit() override;

private:
    void applyDesignResolution();

    std::unique_ptr<hg::AppContext> mContext;
};
