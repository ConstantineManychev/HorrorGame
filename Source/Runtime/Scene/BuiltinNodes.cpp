#include "Runtime/Scene/NodeFactory.h"

#include "Core/Base/ValueConvert.h"
#include "Runtime/App/Log.h"
#include "Runtime/Scene/AxConvert.h"

#include "ui/UIButton.h"

namespace hg
{
    namespace
    {
        constexpr float kPlaceholderSize = 128.0f;

        void noop(ax::Node*, const Value&, BuildContext&)
        {
        }

        bool fileExists(const std::string& aPath)
        {
            return !aPath.empty() && ax::FileUtils::getInstance()->isFileExist(aPath);
        }

        void showPlaceholder(ax::Sprite* aSprite, const std::string& aReason)
        {
            Log::warning(aReason);
            aSprite->setTexture(nullptr);
            aSprite->setTextureRect(ax::Rect(0.0f, 0.0f, kPlaceholderSize, kPlaceholderSize));
            aSprite->setColor(ax::Color3B::MAGENTA);
        }

        ax::Node* createContainer()
        {
            auto* node = ax::Node::create();
            node->setCascadeOpacityEnabled(true);
            node->setCascadeColorEnabled(true);
            return node;
        }

        ax::Node* createAreaNode()
        {
            auto* node = createContainer();
            node->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
            return node;
        }

        void registerNodeProperties(NodeFactory& aFactory)
        {
            aFactory.registerProperty("Node", "position", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setPosition(toAx(readVec2(aValue, {})));
            });
            aFactory.registerProperty("Node", "rotation", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setRotation(aValue.asFloat());
            });
            aFactory.registerProperty("Node", "scale", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                const Vec2 scale = readVec2(aValue, {1.0f, 1.0f});
                aNode->setScaleX(scale.x);
                aNode->setScaleY(scale.y);
            });
            aFactory.registerProperty("Node", "z", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setLocalZOrder(static_cast<int>(aValue.asInt()));
            });
            aFactory.registerProperty("Node", "visible", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setVisible(aValue.asBool(true));
            });
            aFactory.registerProperty("Node", "opacity", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setOpacity(static_cast<uint8_t>(std::clamp<int64_t>(aValue.asInt(255), 0, 255)));
            });
            aFactory.registerProperty("Node", "color", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setColor(toAx3B(readColor(aValue, {})));
            });
            aFactory.registerProperty("Node", "y_sort", noop);
        }

        void registerGroup(NodeFactory& aFactory)
        {
            aFactory.registerType("Group", [](BuildContext&)
            {
                return createContainer();
            });
        }

        void registerSprite(NodeFactory& aFactory)
        {
            aFactory.registerType("Sprite", [](BuildContext&) -> ax::Node*
            {
                auto* sprite = ax::Sprite::create();
                sprite->setTextureRect(ax::Rect::ZERO);
                sprite->setCascadeOpacityEnabled(true);
                sprite->setCascadeColorEnabled(true);
                return sprite;
            });
            aFactory.registerProperty("Sprite", "texture", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                auto* sprite = static_cast<ax::Sprite*>(aNode);
                const std::string path = aValue.asString();
                if (path.empty())
                {
                    return;
                }
                if (!fileExists(path))
                {
                    showPlaceholder(sprite, "Texture not found: " + path);
                    return;
                }
                sprite->setTexture(path);
                if (auto* texture = sprite->getTexture())
                {
                    sprite->setTextureRect(ax::Rect(ax::Vec2::ZERO, texture->getContentSize()));
                }
            });
            aFactory.registerProperty("Sprite", "frame", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                auto* sprite = static_cast<ax::Sprite*>(aNode);
                const std::string name = aValue.asString();
                if (name.empty())
                {
                    return;
                }
                if (auto* frame = ax::SpriteFrameCache::getInstance()->findFrame(name))
                {
                    sprite->setSpriteFrame(frame);
                }
                else
                {
                    showPlaceholder(sprite, "Sprite frame not found: " + name);
                }
            });
            aFactory.registerProperty("Sprite", "anchor", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setAnchorPoint(toAx(readVec2(aValue, {0.5f, 0.5f})));
            });
            aFactory.registerProperty("Sprite", "flip_x", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                static_cast<ax::Sprite*>(aNode)->setFlippedX(aValue.asBool());
            });
            aFactory.registerProperty("Sprite", "flip_y", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                static_cast<ax::Sprite*>(aNode)->setFlippedY(aValue.asBool());
            });
        }

        void registerColorRect(NodeFactory& aFactory)
        {
            aFactory.registerType("ColorRect", [](BuildContext&) -> ax::Node*
            {
                auto* sprite = ax::Sprite::create();
                sprite->setTexture(nullptr);
                sprite->setCascadeOpacityEnabled(true);
                return sprite;
            });
            aFactory.registerProperty("ColorRect", "size", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                const Vec2 size = readVec2(aValue, {100.0f, 100.0f});
                static_cast<ax::Sprite*>(aNode)->setTextureRect(ax::Rect(0.0f, 0.0f, std::max(0.0f, size.x), std::max(0.0f, size.y)));
            });
            aFactory.registerProperty("ColorRect", "anchor", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setAnchorPoint(toAx(readVec2(aValue, {0.5f, 0.5f})));
            });
        }

        void registerLabel(NodeFactory& aFactory)
        {
            aFactory.registerType("Label", [](BuildContext&) -> ax::Node*
            {
                auto* label = ax::Label::createWithTTF("", "fonts/arial.ttf", 48.0f);
                label->setCascadeOpacityEnabled(true);
                return label;
            });
            aFactory.registerProperty("Label", "text", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                static_cast<ax::Label*>(aNode)->setString(aValue.asString());
            });
            aFactory.registerProperty("Label", "font", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                auto* label = static_cast<ax::Label*>(aNode);
                const std::string path = aValue.asString();
                if (!fileExists(path))
                {
                    Log::warning("Font not found: " + path);
                    return;
                }
                ax::TTFConfig config = label->getTTFConfig();
                config.fontFilePath = path;
                label->setTTFConfig(config);
            });
            aFactory.registerProperty("Label", "font_size", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                auto* label = static_cast<ax::Label*>(aNode);
                ax::TTFConfig config = label->getTTFConfig();
                config.fontSize = std::max(1.0f, aValue.asFloat(48.0f));
                label->setTTFConfig(config);
            });
            aFactory.registerProperty("Label", "anchor", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                aNode->setAnchorPoint(toAx(readVec2(aValue, {0.5f, 0.5f})));
            });
            aFactory.registerProperty("Label", "align", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                const std::string align = aValue.asString("center");
                auto alignment = ax::TextHAlignment::CENTER;
                if (align == "left")
                {
                    alignment = ax::TextHAlignment::LEFT;
                }
                else if (align == "right")
                {
                    alignment = ax::TextHAlignment::RIGHT;
                }
                static_cast<ax::Label*>(aNode)->setAlignment(alignment);
            });
            aFactory.registerProperty("Label", "max_width", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                static_cast<ax::Label*>(aNode)->setMaxLineWidth(std::max(0.0f, aValue.asFloat()));
            });
        }

        void registerButton(NodeFactory& aFactory)
        {
            aFactory.registerType("Button", [](BuildContext& aContext) -> ax::Node*
            {
                auto* button = ax::ui::Button::create();
                button->setCascadeOpacityEnabled(true);
                button->setTouchEnabled(aContext.mode == BuildMode::Play);
                return button;
            });

            auto textureApplier = [](void (ax::ui::Button::*aLoader)(std::string_view, ax::ui::Widget::TextureResType))
            {
                return [aLoader](ax::Node* aNode, const Value& aValue, BuildContext&)
                {
                    const std::string path = aValue.asString();
                    if (path.empty())
                    {
                        return;
                    }
                    if (!fileExists(path))
                    {
                        Log::warning("Button texture not found: " + path);
                        return;
                    }
                    (static_cast<ax::ui::Button*>(aNode)->*aLoader)(path, ax::ui::Widget::TextureResType::LOCAL);
                };
            };
            aFactory.registerProperty("Button", "normal", textureApplier(&ax::ui::Button::loadTextureNormal));
            aFactory.registerProperty("Button", "pressed", textureApplier(&ax::ui::Button::loadTexturePressed));
            aFactory.registerProperty("Button", "disabled", textureApplier(&ax::ui::Button::loadTextureDisabled));

            aFactory.registerProperty("Button", "text", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                static_cast<ax::ui::Button*>(aNode)->setTitleText(aValue.asString());
            });
            aFactory.registerProperty("Button", "font", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                if (fileExists(aValue.asString()))
                {
                    static_cast<ax::ui::Button*>(aNode)->setTitleFontName(aValue.asString());
                }
            });
            aFactory.registerProperty("Button", "font_size", [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                static_cast<ax::ui::Button*>(aNode)->setTitleFontSize(std::max(1.0f, aValue.asFloat(48.0f)));
            });
            aFactory.registerProperty("Button", "enabled", [](ax::Node* aNode, const Value& aValue, BuildContext& aContext)
            {
                auto* button = static_cast<ax::ui::Button*>(aNode);
                button->setBright(aValue.asBool(true));
                button->setTouchEnabled(aContext.mode == BuildMode::Play && aValue.asBool(true));
            });
            aFactory.registerProperty("Button", "on_click", [](ax::Node* aNode, const Value& aValue, BuildContext& aContext)
            {
                if (aContext.mode != BuildMode::Play)
                {
                    return;
                }
                auto runActions = aContext.runActions;
                static_cast<ax::ui::Button*>(aNode)->addClickEventListener([runActions, aValue](ax::Object*)
                {
                    if (runActions)
                    {
                        runActions(aValue);
                    }
                });
            });
        }

        void registerHelpers(NodeFactory& aFactory)
        {
            aFactory.registerType("ParallaxLayer", [](BuildContext&)
            {
                return createContainer();
            });
            aFactory.registerProperty("ParallaxLayer", "factor", noop);

            aFactory.registerType("Path", [](BuildContext&)
            {
                return createContainer();
            });
            for (const char* property : {"kind", "points", "handles", "closed"})
            {
                aFactory.registerProperty("Path", property, noop);
            }

            aFactory.registerType("SpawnPoint", [](BuildContext&)
            {
                return createContainer();
            });
            aFactory.registerProperty("SpawnPoint", "prefab", noop);
            aFactory.registerProperty("SpawnPoint", "player", noop);

            auto sizeApplier = [](ax::Node* aNode, const Value& aValue, BuildContext&)
            {
                const Vec2 size = readVec2(aValue, {});
                aNode->setContentSize(ax::Size(std::max(0.0f, size.x), std::max(0.0f, size.y)));
            };

            aFactory.registerType("Trigger", [](BuildContext&)
            {
                return createAreaNode();
            });
            aFactory.registerProperty("Trigger", "size", sizeApplier);
            for (const char* property : {"once", "on_enter", "on_exit"})
            {
                aFactory.registerProperty("Trigger", property, noop);
            }

            aFactory.registerType("Collider", [](BuildContext&)
            {
                return createAreaNode();
            });
            aFactory.registerProperty("Collider", "size", sizeApplier);
            aFactory.registerProperty("Collider", "one_way", noop);
        }
    }

    void registerBuiltinNodes(NodeFactory& aFactory)
    {
        registerNodeProperties(aFactory);
        registerGroup(aFactory);
        registerSprite(aFactory);
        registerColorRect(aFactory);
        registerLabel(aFactory);
        registerButton(aFactory);
        registerHelpers(aFactory);
    }
}
