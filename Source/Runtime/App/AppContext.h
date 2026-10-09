#pragma once

#include "Core/App/AppConfig.h"
#include "Core/Base/Diagnostics.h"
#include "Core/Scene/Schema.h"
#include "Runtime/App/AudioService.h"
#include "Runtime/App/ContentStore.h"
#include "Runtime/App/InputService.h"
#include "Runtime/Scene/NodeFactory.h"

namespace hg
{
    class AppContext
    {
    public:
        AppContext();
        ~AppContext();

        AppContext(const AppContext&) = delete;
        AppContext& operator=(const AppContext&) = delete;

        bool initialize(Diagnostics& aDiagnostics);
        void preloadAtlases();

        const AppConfig& config() const;
        const SchemaRegistry& schema() const;
        ContentStore& content();
        InputService& input();
        AudioService& audio();
        NodeFactory& nodes();

    private:
        AppConfig mConfig;
        ContentStore mContent;
        InputService mInput;
        AudioService mAudio;
        NodeFactory mNodes;
    };
}
