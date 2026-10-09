#include "Runtime/Scene/NodeFactory.h"

#include "Runtime/App/Log.h"

namespace hg
{
    NodeFactory::NodeFactory(const SchemaRegistry& aSchema)
        : mSchema(aSchema)
    {
        registerBuiltinNodes(*this);
    }

    void NodeFactory::registerType(const std::string& aTypeId, Creator aCreator)
    {
        mTypes[aTypeId].creator = std::move(aCreator);
    }

    void NodeFactory::registerProperty(const std::string& aTypeId, const std::string& aPropertyId, Applier aApplier)
    {
        mTypes[aTypeId].appliers[aPropertyId] = std::move(aApplier);
    }

    ax::Node* NodeFactory::create(const std::string& aTypeId, BuildContext& aContext) const
    {
        auto it = mTypes.find(aTypeId);
        if (it == mTypes.end() || !it->second.creator)
        {
            Log::error("No node binding for type '" + aTypeId + "'");
            return ax::Node::create();
        }
        return it->second.creator(aContext);
    }

    bool NodeFactory::applyProperty(ax::Node* aNode, const std::string& aTypeId, const std::string& aPropertyId, const Value* aValue, BuildContext& aContext) const
    {
        if (!aNode)
        {
            return false;
        }
        const Applier* applier = findApplier(aTypeId, aPropertyId);
        if (!applier)
        {
            return false;
        }
        if (aValue)
        {
            (*applier)(aNode, *aValue, aContext);
            return true;
        }
        if (const PropertySchema* property = mSchema.findProperty(aTypeId, aPropertyId))
        {
            (*applier)(aNode, property->defaultValue, aContext);
            return true;
        }
        return false;
    }

    void NodeFactory::applyAll(ax::Node* aNode, const std::string& aTypeId, const ValueObject& aProps, BuildContext& aContext) const
    {
        for (const PropertySchema* property : mSchema.collectProperties(aTypeId))
        {
            if (const Applier* applier = findApplier(aTypeId, property->id))
            {
                (*applier)(aNode, propertyValueOrDefault(aProps, *property), aContext);
            }
        }
    }

    std::vector<std::string> NodeFactory::findUnboundProperties() const
    {
        std::vector<std::string> missing;
        for (const auto& type : mSchema.types())
        {
            if (!type.creatable)
            {
                continue;
            }
            if (mTypes.find(type.id) == mTypes.end())
            {
                missing.push_back(type.id);
                continue;
            }
            for (const PropertySchema* property : mSchema.collectProperties(type.id))
            {
                if (!findApplier(type.id, property->id))
                {
                    missing.push_back(type.id + "." + property->id);
                }
            }
        }
        return missing;
    }

    const SchemaRegistry& NodeFactory::schema() const
    {
        return mSchema;
    }

    const NodeFactory::Applier* NodeFactory::findApplier(const std::string& aTypeId, const std::string& aPropertyId) const
    {
        std::string current = aTypeId;
        for (int depth = 0; depth < 16 && !current.empty(); ++depth)
        {
            auto type = mTypes.find(current);
            if (type != mTypes.end())
            {
                auto applier = type->second.appliers.find(aPropertyId);
                if (applier != type->second.appliers.end())
                {
                    return &applier->second;
                }
            }
            const TypeSchema* schema = mSchema.findType(current);
            current = schema ? schema->base : std::string();
        }
        return nullptr;
    }
}
