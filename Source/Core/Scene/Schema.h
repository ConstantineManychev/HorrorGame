#pragma once

#include "Core/Base/Value.h"

#include <string>
#include <string_view>
#include <vector>

namespace hg
{
    enum class PropertyType
    {
        Bool,
        Int,
        Float,
        Vec2,
        Color,
        String,
        Text,
        Asset,
        SpriteFrame,
        Enum,
        ObjectRef,
        SceneRef,
        TimelineRef,
        ActionList,
        PointList,
        Any
    };

    enum class SceneKind
    {
        Location,
        Screen
    };

    struct PropertySchema
    {
        std::string id;
        std::string label;
        PropertyType type = PropertyType::Any;
        Value defaultValue;
        bool animatable = false;
        bool hidden = false;
        float minValue = 0.0f;
        float maxValue = 0.0f;
        float step = 0.0f;
        std::vector<std::string> options;
        std::string refType;
        std::string tooltip;

        bool hasRange() const
        {
            return maxValue > minValue;
        }
    };

    struct TypeSchema
    {
        std::string id;
        std::string base;
        std::string category;
        std::string description;
        bool allowsChildren = true;
        bool creatable = true;
        std::vector<PropertySchema> properties;
    };

    struct ComponentSchema
    {
        std::string id;
        std::string description;
        std::vector<PropertySchema> properties;
    };

    struct ActionSchema
    {
        std::string id;
        std::string description;
        std::vector<PropertySchema> arguments;
    };

    class SchemaRegistry
    {
    public:
        void addType(TypeSchema aType);
        void addComponent(ComponentSchema aComponent);
        void addAction(ActionSchema aAction);
        void setSceneSettings(std::vector<PropertySchema> aSettings);
        void setCameraProperties(std::vector<PropertySchema> aProperties);

        const TypeSchema* findType(std::string_view aId) const;
        const ComponentSchema* findComponent(std::string_view aId) const;
        const ActionSchema* findAction(std::string_view aId) const;

        std::vector<const PropertySchema*> collectProperties(std::string_view aTypeId) const;
        const PropertySchema* findProperty(std::string_view aTypeId, std::string_view aPropertyId) const;
        const PropertySchema* findComponentProperty(std::string_view aComponentId, std::string_view aPropertyId) const;
        const PropertySchema* findSetting(std::string_view aId) const;
        const PropertySchema* findCameraProperty(std::string_view aId) const;
        bool isKindOf(std::string_view aTypeId, std::string_view aBaseId) const;

        const std::vector<TypeSchema>& types() const;
        const std::vector<ComponentSchema>& components() const;
        const std::vector<ActionSchema>& actions() const;
        const std::vector<PropertySchema>& sceneSettings() const;
        const std::vector<PropertySchema>& cameraProperties() const;

    private:
        std::vector<TypeSchema> mTypes;
        std::vector<ComponentSchema> mComponents;
        std::vector<ActionSchema> mActions;
        std::vector<PropertySchema> mSceneSettings;
        std::vector<PropertySchema> mCameraProperties;
    };

    const SchemaRegistry& builtinSchemas();

    std::string_view propertyTypeName(PropertyType aType);
    std::string_view sceneKindName(SceneKind aKind);
    bool parseSceneKind(std::string_view aName, SceneKind& aKind);
    bool valueMatchesProperty(const Value& aValue, const PropertySchema& aProperty);
    const Value& propertyValueOrDefault(const ValueObject& aProps, const PropertySchema& aProperty);
}
