#include "Runtime/App/AppContext.h"

#include "Runtime/App/Log.h"

#include "axmol.h"

namespace hg
{
    AppContext::AppContext()
        : mNodes(builtinSchemas())
    {
    }

    AppContext::~AppContext()
    {
        mInput.detach();
    }

    bool AppContext::initialize(Diagnostics& aDiagnostics)
    {
        auto config = mContent.loadAppConfig(aDiagnostics);
        if (config)
        {
            mConfig = std::move(*config);
        }
        else
        {
            mConfig.input = defaultInputBindings();
        }

        mInput.configure(mConfig.input);
        mInput.attach();

        for (const auto& missing : mNodes.findUnboundProperties())
        {
            aDiagnostics.error("node bindings", "schema entry has no runtime binding: " + missing);
        }
        return !aDiagnostics.hasErrors();
    }

    void AppContext::preloadAtlases()
    {
        auto* cache = ax::SpriteFrameCache::getInstance();
        for (const auto& atlas : mConfig.preloadAtlases)
        {
            if (mContent.exists(atlas))
            {
                cache->addSpriteFramesWithFile(atlas);
            }
            else
            {
                Log::warning("Atlas not found: " + atlas);
            }
        }
    }

    const AppConfig& AppContext::config() const
    {
        return mConfig;
    }

    const SchemaRegistry& AppContext::schema() const
    {
        return mNodes.schema();
    }

    ContentStore& AppContext::content()
    {
        return mContent;
    }

    InputService& AppContext::input()
    {
        return mInput;
    }

    AudioService& AppContext::audio()
    {
        return mAudio;
    }

    NodeFactory& AppContext::nodes()
    {
        return mNodes;
    }
}
