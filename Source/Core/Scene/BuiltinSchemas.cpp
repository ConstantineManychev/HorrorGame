#include "Core/Scene/Schema.h"

#include "Core/Animation/Easing.h"

#include <cctype>

namespace hg
{
    namespace
    {
        std::string humanize(std::string_view aId)
        {
            std::string label;
            bool capitalize = true;
            for (char character : aId)
            {
                if (character == '_')
                {
                    label += ' ';
                    capitalize = true;
                    continue;
                }
                label += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(character))) : character;
                capitalize = false;
            }
            return label;
        }

        PropertySchema makeProperty(std::string_view aId, PropertyType aType, Value aDefault)
        {
            PropertySchema property;
            property.id = std::string(aId);
            property.label = humanize(aId);
            property.type = aType;
            property.defaultValue = std::move(aDefault);
            return property;
        }

        PropertySchema animatable(PropertySchema aProperty)
        {
            aProperty.animatable = true;
            return aProperty;
        }

        PropertySchema hidden(PropertySchema aProperty)
        {
            aProperty.hidden = true;
            return aProperty;
        }

        PropertySchema ranged(PropertySchema aProperty, float aMin, float aMax, float aStep)
        {
            aProperty.minValue = aMin;
            aProperty.maxValue = aMax;
            aProperty.step = aStep;
            return aProperty;
        }

        PropertySchema withOptions(PropertySchema aProperty, std::vector<std::string> aOptions)
        {
            aProperty.options = std::move(aOptions);
            return aProperty;
        }

        PropertySchema refersTo(PropertySchema aProperty, std::string aType)
        {
            aProperty.refType = std::move(aType);
            return aProperty;
        }

        PropertySchema described(PropertySchema aProperty, std::string aTooltip)
        {
            aProperty.tooltip = std::move(aTooltip);
            return aProperty;
        }

        Value vec2(float aX, float aY)
        {
            return Value(ValueArray{Value(aX), Value(aY)});
        }

        Value color(int aR, int aG, int aB)
        {
            return Value(ValueArray{Value(aR), Value(aG), Value(aB)});
        }

        Value emptyList()
        {
            return Value(ValueArray{});
        }

        std::vector<std::string> imageExtensions()
        {
            return {".png", ".jpg", ".jpeg", ".webp"};
        }

        std::vector<std::string> audioExtensions()
        {
            return {".ogg", ".mp3", ".wav"};
        }

        std::vector<std::string> controlModes()
        {
            return {"none", "side_scroll", "top_down"};
        }

        std::vector<std::string> easeNames()
        {
            std::vector<std::string> names;
            for (EaseType type : allEaseTypes())
            {
                if (type != EaseType::Bezier)
                {
                    names.emplace_back(easeTypeName(type));
                }
            }
            return names;
        }

        void registerTypes(SchemaRegistry& aRegistry)
        {
            TypeSchema node;
            node.id = "Node";
            node.category = "Base";
            node.creatable = false;
            node.properties = {
                animatable(makeProperty("position", PropertyType::Vec2, vec2(0.0f, 0.0f))),
                animatable(makeProperty("rotation", PropertyType::Float, Value(0.0))),
                animatable(makeProperty("scale", PropertyType::Vec2, vec2(1.0f, 1.0f))),
                makeProperty("z", PropertyType::Int, Value(0)),
                animatable(makeProperty("visible", PropertyType::Bool, Value(true))),
                animatable(ranged(makeProperty("opacity", PropertyType::Int, Value(255)), 0.0f, 255.0f, 1.0f)),
                animatable(makeProperty("color", PropertyType::Color, color(255, 255, 255))),
                described(makeProperty("y_sort", PropertyType::Bool, Value(false)), "Sort draw order by Y in top-down mode"),
            };
            aRegistry.addType(std::move(node));

            TypeSchema group;
            group.id = "Group";
            group.base = "Node";
            group.category = "Basic";
            group.description = "Empty container for other objects";
            aRegistry.addType(std::move(group));

            TypeSchema sprite;
            sprite.id = "Sprite";
            sprite.base = "Node";
            sprite.category = "Visual";
            sprite.description = "Image from a texture file or an atlas frame";
            sprite.properties = {
                animatable(withOptions(makeProperty("texture", PropertyType::Asset, Value("")), imageExtensions())),
                animatable(described(makeProperty("frame", PropertyType::SpriteFrame, Value("")), "Sprite frame name from a preloaded atlas, overrides texture")),
                ranged(makeProperty("anchor", PropertyType::Vec2, vec2(0.5f, 0.5f)), 0.0f, 1.0f, 0.01f),
                makeProperty("flip_x", PropertyType::Bool, Value(false)),
                makeProperty("flip_y", PropertyType::Bool, Value(false)),
            };
            aRegistry.addType(std::move(sprite));

            TypeSchema rect;
            rect.id = "ColorRect";
            rect.base = "Node";
            rect.category = "Visual";
            rect.description = "Solid rectangle, useful for prototyping";
            rect.properties = {
                animatable(makeProperty("size", PropertyType::Vec2, vec2(100.0f, 100.0f))),
                ranged(makeProperty("anchor", PropertyType::Vec2, vec2(0.5f, 0.5f)), 0.0f, 1.0f, 0.01f),
            };
            aRegistry.addType(std::move(rect));

            TypeSchema label;
            label.id = "Label";
            label.base = "Node";
            label.category = "Visual";
            label.description = "Text rendered with a TTF font";
            label.properties = {
                animatable(makeProperty("text", PropertyType::Text, Value("Text"))),
                withOptions(makeProperty("font", PropertyType::Asset, Value("fonts/arial.ttf")), {".ttf", ".otf"}),
                ranged(makeProperty("font_size", PropertyType::Float, Value(48.0)), 4.0f, 512.0f, 1.0f),
                ranged(makeProperty("anchor", PropertyType::Vec2, vec2(0.5f, 0.5f)), 0.0f, 1.0f, 0.01f),
                withOptions(makeProperty("align", PropertyType::Enum, Value("center")), {"left", "center", "right"}),
                described(makeProperty("max_width", PropertyType::Float, Value(0.0)), "Wrap width in world units, 0 disables wrapping"),
            };
            aRegistry.addType(std::move(label));

            TypeSchema button;
            button.id = "Button";
            button.base = "Node";
            button.category = "UI";
            button.description = "Clickable image with optional caption";
            button.properties = {
                withOptions(makeProperty("normal", PropertyType::Asset, Value("")), imageExtensions()),
                withOptions(makeProperty("pressed", PropertyType::Asset, Value("")), imageExtensions()),
                withOptions(makeProperty("disabled", PropertyType::Asset, Value("")), imageExtensions()),
                makeProperty("text", PropertyType::Text, Value("")),
                withOptions(makeProperty("font", PropertyType::Asset, Value("fonts/arial.ttf")), {".ttf", ".otf"}),
                ranged(makeProperty("font_size", PropertyType::Float, Value(48.0)), 4.0f, 512.0f, 1.0f),
                makeProperty("enabled", PropertyType::Bool, Value(true)),
                makeProperty("on_click", PropertyType::ActionList, emptyList()),
            };
            aRegistry.addType(std::move(button));

            TypeSchema parallax;
            parallax.id = "ParallaxLayer";
            parallax.base = "Node";
            parallax.category = "Layout";
            parallax.description = "Moves children slower or faster than the camera";
            parallax.properties = {
                ranged(described(makeProperty("factor", PropertyType::Vec2, vec2(1.0f, 1.0f)), "1 moves with the world, 0 stays fixed on screen"), 0.0f, 2.0f, 0.01f),
            };
            aRegistry.addType(std::move(parallax));

            TypeSchema path;
            path.id = "Path";
            path.base = "Node";
            path.category = "Gameplay";
            path.description = "Curve for camera and object motion";
            path.allowsChildren = false;
            path.properties = {
                withOptions(makeProperty("kind", PropertyType::Enum, Value("catmull_rom")), {"linear", "catmull_rom", "bezier"}),
                makeProperty("points", PropertyType::PointList, Value(ValueArray{vec2(0.0f, 0.0f), vec2(400.0f, 0.0f)})),
                hidden(makeProperty("handles", PropertyType::Any, Value())),
                makeProperty("closed", PropertyType::Bool, Value(false)),
            };
            aRegistry.addType(std::move(path));

            TypeSchema spawn;
            spawn.id = "SpawnPoint";
            spawn.base = "Node";
            spawn.category = "Gameplay";
            spawn.description = "Spawns a prefab when the scene starts";
            spawn.allowsChildren = false;
            spawn.properties = {
                withOptions(makeProperty("prefab", PropertyType::Asset, Value("prefabs/player.json")), {".json"}),
                described(makeProperty("player", PropertyType::Bool, Value(true)), "Spawned object becomes the controlled player and camera target"),
            };
            aRegistry.addType(std::move(spawn));

            TypeSchema trigger;
            trigger.id = "Trigger";
            trigger.base = "Node";
            trigger.category = "Gameplay";
            trigger.description = "Runs actions when the player enters or leaves the area";
            trigger.properties = {
                makeProperty("size", PropertyType::Vec2, vec2(200.0f, 200.0f)),
                makeProperty("once", PropertyType::Bool, Value(true)),
                makeProperty("on_enter", PropertyType::ActionList, emptyList()),
                makeProperty("on_exit", PropertyType::ActionList, emptyList()),
            };
            aRegistry.addType(std::move(trigger));

            TypeSchema collider;
            collider.id = "Collider";
            collider.base = "Node";
            collider.category = "Gameplay";
            collider.description = "Static solid box for character movement";
            collider.allowsChildren = false;
            collider.properties = {
                makeProperty("size", PropertyType::Vec2, vec2(400.0f, 40.0f)),
                described(makeProperty("one_way", PropertyType::Bool, Value(false)), "Side-scroll platform passable from below"),
            };
            aRegistry.addType(std::move(collider));
        }

        void registerComponents(SchemaRegistry& aRegistry)
        {
            ComponentSchema body;
            body.id = "CharacterBody";
            body.description = "Collision box used by character controllers";
            body.properties = {
                makeProperty("size", PropertyType::Vec2, vec2(80.0f, 160.0f)),
                described(makeProperty("offset", PropertyType::Vec2, vec2(0.0f, 80.0f)), "Box center relative to the object position"),
            };
            aRegistry.addComponent(std::move(body));

            ComponentSchema controller;
            controller.id = "PlayerController";
            controller.description = "Moves the object with player input in side-scroll and top-down modes";
            controller.properties = {
                makeProperty("run_speed", PropertyType::Float, Value(520.0)),
                makeProperty("acceleration", PropertyType::Float, Value(4000.0)),
                makeProperty("jump_speed", PropertyType::Float, Value(1250.0)),
                makeProperty("gravity", PropertyType::Float, Value(3600.0)),
                makeProperty("max_fall_speed", PropertyType::Float, Value(2200.0)),
                makeProperty("coyote_time", PropertyType::Float, Value(0.1)),
                makeProperty("jump_buffer", PropertyType::Float, Value(0.12)),
                makeProperty("walk_speed", PropertyType::Float, Value(420.0)),
                makeProperty("walk_acceleration", PropertyType::Float, Value(3200.0)),
            };
            aRegistry.addComponent(std::move(controller));

            ComponentSchema follower;
            follower.id = "PathFollower";
            follower.description = "Moves the object along a Path object";
            follower.properties = {
                refersTo(makeProperty("path", PropertyType::ObjectRef, Value(0)), "Path"),
                ranged(makeProperty("duration", PropertyType::Float, Value(4.0)), 0.01f, 600.0f, 0.1f),
                withOptions(makeProperty("mode", PropertyType::Enum, Value("loop")), {"once", "loop", "ping_pong"}),
                withOptions(makeProperty("ease", PropertyType::Enum, Value("linear")), easeNames()),
                makeProperty("orient", PropertyType::Bool, Value(false)),
                makeProperty("autostart", PropertyType::Bool, Value(true)),
            };
            aRegistry.addComponent(std::move(follower));
        }

        void registerActions(SchemaRegistry& aRegistry)
        {
            aRegistry.addAction({"change_scene", "Switch to another scene", {makeProperty("scene", PropertyType::SceneRef, Value(""))}});
            aRegistry.addAction({"play_timeline", "Play a timeline of the current scene", {makeProperty("timeline", PropertyType::TimelineRef, Value(""))}});
            aRegistry.addAction({"stop_timeline", "Stop a timeline of the current scene", {makeProperty("timeline", PropertyType::TimelineRef, Value(""))}});
            aRegistry.addAction({"set_control_mode", "Switch player control scheme", {withOptions(makeProperty("mode", PropertyType::Enum, Value("side_scroll")), controlModes())}});
            aRegistry.addAction({"set_visible", "Show or hide an object", {makeProperty("target", PropertyType::ObjectRef, Value(0)), makeProperty("visible", PropertyType::Bool, Value(true))}});
            aRegistry.addAction({"play_sound", "Play a sound effect", {withOptions(makeProperty("file", PropertyType::Asset, Value("")), audioExtensions())}});
            aRegistry.addAction({"play_music", "Play background music", {withOptions(makeProperty("file", PropertyType::Asset, Value("")), audioExtensions()), makeProperty("loop", PropertyType::Bool, Value(true))}});
            aRegistry.addAction({"stop_music", "Stop background music", {}});
            aRegistry.addAction({"set_input_enabled", "Enable or disable player input", {makeProperty("enabled", PropertyType::Bool, Value(true))}});
            aRegistry.addAction({"quit_game", "Exit the application", {}});
        }

        void registerSettings(SchemaRegistry& aRegistry)
        {
            aRegistry.setSceneSettings({
                makeProperty("world_size", PropertyType::Vec2, vec2(3840.0f, 2160.0f)),
                ranged(described(makeProperty("view_height", PropertyType::Float, Value(2160.0)), "World units visible vertically at zoom 1"), 100.0f, 20000.0f, 10.0f),
                makeProperty("background", PropertyType::Color, color(0, 0, 0)),
                withOptions(makeProperty("control_mode", PropertyType::Enum, Value("none")), controlModes()),
                ranged(described(makeProperty("camera_dead_zone", PropertyType::Vec2, vec2(0.15f, 0.2f)), "Fraction of the view where the target moves without camera motion"), 0.0f, 1.0f, 0.01f),
                ranged(described(makeProperty("camera_damping", PropertyType::Float, Value(6.0)), "Camera follow smoothing, 0 snaps instantly"), 0.0f, 60.0f, 0.1f),
                withOptions(makeProperty("music", PropertyType::Asset, Value("")), audioExtensions()),
                makeProperty("on_start", PropertyType::ActionList, emptyList()),
            });

            aRegistry.setCameraProperties({
                animatable(makeProperty("center", PropertyType::Vec2, vec2(0.0f, 0.0f))),
                animatable(ranged(makeProperty("zoom", PropertyType::Float, Value(1.0)), 0.05f, 20.0f, 0.01f)),
                animatable(makeProperty("rotation", PropertyType::Float, Value(0.0))),
                animatable(described(makeProperty("shake", PropertyType::Float, Value(0.0)), "Shake amplitude in world units")),
                animatable(makeProperty("shake_frequency", PropertyType::Float, Value(18.0))),
            });
        }

        SchemaRegistry createBuiltinSchemas()
        {
            SchemaRegistry registry;
            registerTypes(registry);
            registerComponents(registry);
            registerActions(registry);
            registerSettings(registry);
            return registry;
        }
    }

    const SchemaRegistry& builtinSchemas()
    {
        static const SchemaRegistry registry = createBuiltinSchemas();
        return registry;
    }
}
