#include "EntityFactory.h"
#include "Basics/Player.h"
#include "Basics/Enemy.h"
#include "Basics/BaseLocation.h"
#include "Basics/Parallax.h"
#include "ui/UIButton.h"
#include "Constants.h"
#include "Components/MovementComponent.h"
#include "Components/InputComponent.h"

USING_NS_CC;
_CSTART

EntityFactory* EntityFactory::getInstance()
{
	static EntityFactory instance;
	return &instance;
}

EntityFactory::EntityFactory()
{
	registerEntity(Constants::Types::NODE, []() { return Node::create(); });
	registerEntity(Constants::Types::SPRITE, []() { return Sprite::create(); });
	registerEntity(Constants::Types::BUTTON, []() { return ui::Button::create(); });
	registerEntity(Constants::Types::LOCATION, []() { return BaseLocation::create(); });
	registerEntity(Constants::Types::PARALLAX, []() { return Parallax::create(); });

	registerEntity(Constants::Types::PLAYER, []() { return Player::create(); });
	registerEntity(Constants::Types::ENEMY, []() { return Enemy::create(); });
}

void EntityFactory::registerEntity(const std::string& type, EntityCreator creator)
{
	mCreators[type] = creator;
}

Node* EntityFactory::createEntity(const std::string& type)
{
	auto it = mCreators.find(type);
	if (it != mCreators.end())
	{
		return it->second();
	}
	CCLOG("EntityFactory: Unknown entity type '%s', returning empty Node.", type.c_str());
	return Node::create();
}

Node* EntityFactory::createEntityFromConfig(const ValueMap& config)
{
	Node* entity = Node::create();

	if (config.count("components"))
	{
		const auto& components = config.at("components").asValueVector();
		for (const auto& compVal : components)
		{
			ValueMap compConfig = compVal.asValueMap();
			std::string type = compConfig["type"].asString();

			if (type == "SpriteComponent") {
				auto sprite = Sprite::create();
				if (compConfig.count("sprite_frame")) sprite->setSpriteFrame(compConfig["sprite_frame"].asString());
				entity->addChild(sprite);
			}
			else if (type == "MovementComponent") {
				auto move = MovementComponent::create();
				entity->addComponent(move);
			}
			else if (type == "InputComponent") {
				auto input = InputComponent::create();
				entity->addComponent(input);
			}
		}
	}
	return entity;
}

_CEND