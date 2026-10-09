#include "Core/Scene/Schema.h"

#include "Core/Base/ValueConvert.h"

#include <algorithm>

namespace hg
{
    namespace
    {
        template <typename T>
        const T* findById(const std::vector<T>& aItems, std::string_view aId)
        {
            auto it = std::find_if(aItems.begin(), aItems.end(), [aId](const T& aItem)
            {
                return aItem.id == aId;
            });
            return it != aItems.end() ? &*it : nullptr;
        }

        template <typename T>
        void addOrReplace(std::vector<T>& aItems, T aItem)
        {
            auto it = std::find_if(aItems.begin(), aItems.end(), [&aItem](const T& aExisting)
            {
                return aExisting.id == aItem.id;
            });
            if (it != aItems.end())
            {
                *it = std::move(aItem);
            }
            else
            {
                aItems.push_back(std::move(aItem));
            }
        }

        const PropertySchema* findPropertyIn(const std::vector<PropertySchema>& aProperties, std::string_view aId)
        {
            return findById(aProperties, aId);
        }
    }

    void SchemaRegistry::addType(TypeSchema aType)
    {
        addOrReplace(mTypes, std::move(aType));
    }

    void SchemaRegistry::addComponent(ComponentSchema aComponent)
    {
        addOrReplace(mComponents, std::move(aComponent));
    }

    void SchemaRegistry::addAction(ActionSchema aAction)
    {
        addOrReplace(mActions, std::move(aAction));
    }

    void SchemaRegistry::setSceneSettings(std::vector<PropertySchema> aSettings)
    {
        mSceneSettings = std::move(aSettings);
    }

    void SchemaRegistry::setCameraProperties(std::vector<PropertySchema> aProperties)
    {
        mCameraProperties = std::move(aProperties);
    }

    const TypeSchema* SchemaRegistry::findType(std::string_view aId) const
    {
        return findById(mTypes, aId);
    }

    const ComponentSchema* SchemaRegistry::findComponent(std::string_view aId) const
    {
        return findById(mComponents, aId);
    }

    const ActionSchema* SchemaRegistry::findAction(std::string_view aId) const
    {
        return findById(mActions, aId);
    }

    std::vector<const PropertySchema*> SchemaRegistry::collectProperties(std::string_view aTypeId) const
    {
        std::vector<const TypeSchema*> chain;
        const TypeSchema* current = findType(aTypeId);
        while (current && chain.size() < 16)
        {
            chain.push_back(current);
            current = current->base.empty() ? nullptr : findType(current->base);
        }

        std::vector<const PropertySchema*> properties;
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        {
            for (const auto& property : (*it)->properties)
            {
                auto existing = std::find_if(properties.begin(), properties.end(), [&property](const PropertySchema* aExisting)
                {
                    return aExisting->id == property.id;
                });
                if (existing != properties.end())
                {
                    *existing = &property;
                }
                else
                {
                    properties.push_back(&property);
                }
            }
        }
        return properties;
    }

    const PropertySchema* SchemaRegistry::findProperty(std::string_view aTypeId, std::string_view aPropertyId) const
    {
        const TypeSchema* current = findType(aTypeId);
        int depth = 0;
        while (current && depth < 16)
        {
            if (const PropertySchema* property = findPropertyIn(current->properties, aPropertyId))
            {
                return property;
            }
            current = current->base.empty() ? nullptr : findType(current->base);
            ++depth;
        }
        return nullptr;
    }

    const PropertySchema* SchemaRegistry::findComponentProperty(std::string_view aComponentId, std::string_view aPropertyId) const
    {
        const ComponentSchema* component = findComponent(aComponentId);
        return component ? findPropertyIn(component->properties, aPropertyId) : nullptr;
    }

    const PropertySchema* SchemaRegistry::findSetting(std::string_view aId) const
    {
        return findPropertyIn(mSceneSettings, aId);
    }

    const PropertySchema* SchemaRegistry::findCameraProperty(std::string_view aId) const
    {
        return findPropertyIn(mCameraProperties, aId);
    }

    bool SchemaRegistry::isKindOf(std::string_view aTypeId, std::string_view aBaseId) const
    {
        const TypeSchema* current = findType(aTypeId);
        int depth = 0;
        while (current && depth < 16)
        {
            if (current->id == aBaseId)
            {
                return true;
            }
            current = current->base.empty() ? nullptr : findType(current->base);
            ++depth;
        }
        return false;
    }

    const std::vector<TypeSchema>& SchemaRegistry::types() const
    {
        return mTypes;
    }

    const std::vector<ComponentSchema>& SchemaRegistry::components() const
    {
        return mComponents;
    }

    const std::vector<ActionSchema>& SchemaRegistry::actions() const
    {
        return mActions;
    }

    const std::vector<PropertySchema>& SchemaRegistry::sceneSettings() const
    {
        return mSceneSettings;
    }

    const std::vector<PropertySchema>& SchemaRegistry::cameraProperties() const
    {
        return mCameraProperties;
    }

    std::string_view propertyTypeName(PropertyType aType)
    {
        switch (aType)
        {
        case PropertyType::Bool:
            return "bool";
        case PropertyType::Int:
            return "int";
        case PropertyType::Float:
            return "float";
        case PropertyType::Vec2:
            return "vec2";
        case PropertyType::Color:
            return "color";
        case PropertyType::String:
            return "string";
        case PropertyType::Text:
            return "text";
        case PropertyType::Asset:
            return "asset";
        case PropertyType::SpriteFrame:
            return "sprite_frame";
        case PropertyType::Enum:
            return "enum";
        case PropertyType::ObjectRef:
            return "object_ref";
        case PropertyType::SceneRef:
            return "scene_ref";
        case PropertyType::TimelineRef:
            return "timeline_ref";
        case PropertyType::ActionList:
            return "action_list";
        case PropertyType::PointList:
            return "point_list";
        case PropertyType::Any:
            return "any";
        }
        return "any";
    }

    std::string_view sceneKindName(SceneKind aKind)
    {
        return aKind == SceneKind::Location ? "location" : "screen";
    }

    bool parseSceneKind(std::string_view aName, SceneKind& aKind)
    {
        if (aName == "location")
        {
            aKind = SceneKind::Location;
            return true;
        }
        if (aName == "screen")
        {
            aKind = SceneKind::Screen;
            return true;
        }
        return false;
    }

    bool valueMatchesProperty(const Value& aValue, const PropertySchema& aProperty)
    {
        switch (aProperty.type)
        {
        case PropertyType::Bool:
            return aValue.isBool();
        case PropertyType::Int:
        case PropertyType::Float:
            return aValue.isNumber();
        case PropertyType::Vec2:
            return readVec2(aValue).has_value() && aValue.isArray();
        case PropertyType::Color:
            return readColor(aValue).has_value();
        case PropertyType::String:
        case PropertyType::Text:
        case PropertyType::Asset:
        case PropertyType::SpriteFrame:
        case PropertyType::SceneRef:
        case PropertyType::TimelineRef:
            return aValue.isString();
        case PropertyType::Enum:
            return aValue.isString() && (aProperty.options.empty() || std::find(aProperty.options.begin(), aProperty.options.end(), aValue.asString()) != aProperty.options.end());
        case PropertyType::ObjectRef:
            return aValue.isInt() || aValue.isNull();
        case PropertyType::ActionList:
        {
            if (!aValue.isArray())
            {
                return false;
            }
            for (const auto& action : aValue.asArray())
            {
                if (!action.isObject() || !action.asObject().get("do").isString())
                {
                    return false;
                }
            }
            return true;
        }
        case PropertyType::PointList:
        {
            if (!aValue.isArray())
            {
                return false;
            }
            for (const auto& point : aValue.asArray())
            {
                if (!readVec2(point) || !point.isArray())
                {
                    return false;
                }
            }
            return true;
        }
        case PropertyType::Any:
            return true;
        }
        return true;
    }

    const Value& propertyValueOrDefault(const ValueObject& aProps, const PropertySchema& aProperty)
    {
        const Value* value = aProps.find(aProperty.id);
        return value ? *value : aProperty.defaultValue;
    }
}
