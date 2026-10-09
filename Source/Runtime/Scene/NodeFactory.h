#pragma once

#include "Core/Scene/Document.h"
#include "Core/Scene/Schema.h"

#include "axmol.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace hg
{
    class ContentStore;

    enum class BuildMode
    {
        Play,
        Edit
    };

    struct BuildContext
    {
        BuildMode mode = BuildMode::Play;
        ContentStore* content = nullptr;
        std::function<void(const Value&)> runActions;
    };

    class NodeFactory
    {
    public:
        using Creator = std::function<ax::Node*(BuildContext&)>;
        using Applier = std::function<void(ax::Node*, const Value&, BuildContext&)>;

        explicit NodeFactory(const SchemaRegistry& aSchema);

        void registerType(const std::string& aTypeId, Creator aCreator);
        void registerProperty(const std::string& aTypeId, const std::string& aPropertyId, Applier aApplier);

        ax::Node* create(const std::string& aTypeId, BuildContext& aContext) const;
        bool applyProperty(ax::Node* aNode, const std::string& aTypeId, const std::string& aPropertyId, const Value* aValue, BuildContext& aContext) const;
        void applyAll(ax::Node* aNode, const std::string& aTypeId, const ValueObject& aProps, BuildContext& aContext) const;

        std::vector<std::string> findUnboundProperties() const;
        const SchemaRegistry& schema() const;

    private:
        struct TypeBinding
        {
            Creator creator;
            std::unordered_map<std::string, Applier> appliers;
        };

        const Applier* findApplier(const std::string& aTypeId, const std::string& aPropertyId) const;

        const SchemaRegistry& mSchema;
        std::unordered_map<std::string, TypeBinding> mTypes;
    };

    void registerBuiltinNodes(NodeFactory& aFactory);
}
