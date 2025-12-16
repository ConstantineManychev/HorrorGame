#include "EntityFactory.h"
#include "Basics/Player.h"
#include "Basics/Enemy.h"
#include "Basics/BaseLocation.h"
#include "Basics/Parallax.h"
#include "ui/UIButton.h"
#include "Constants.h"
#include "Components/InputComponent.h"
#include "Components/PhysicsBodyComponent.h"
#include "Components/PhysicsMovementComponent.h"
#include "Components/SpriteComponent.h"
#include "Managers/EventBus.h"
#include "Managers/EventData.h"

USING_NS_CC;
_CSTART

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

void EntityFactory::registerEntity(const std::string& aType, EntityCreator aCreator)
{
	mCreators[aType] = aCreator;
}

Node* EntityFactory::createEntity(const std::string& aType)
{
	auto it = mCreators.find(aType);
	if (it != mCreators.end())
	{
		return it->second();
	}
	CCLOG("EntityFactory: Unknown entity type '%s', returning empty Node.", aType.c_str());
	return Node::create();
}

Node* EntityFactory::createEntityFromConfig(const ValueMap& aConfig)
{
	GameEntity* entity = GameEntity::create();
	std::string entityType = "Node";

	if (aConfig.count("type")) {
		entityType = aConfig.at("type").asString();
	}

	if (aConfig.count("components"))
	{
		const auto& componentsVector = aConfig.at("components").asValueVector();
		for (const auto& compVal : componentsVector)
		{
			ValueMap compConfig = compVal.asValueMap();
			std::string compType = compConfig["type"].asString();

			if (compType == "SpriteComponent") {
				auto spriteComp = SpriteComponent::create();
				spriteComp->configure(compConfig);
				entity->addComponent(spriteComp);
			}
			else if (compType == "PhysicsBodyComponent") {
				bool isDynamic = compConfig.count("is_dynamic") ? compConfig["is_dynamic"].asBool() : true;
				auto phys = PhysicsBodyComponent::create(isDynamic);
				if (compConfig.count("category_bitmask")) phys->setCategoryBitmask(compConfig["category_bitmask"].asInt());
				if (compConfig.count("contact_test_bitmask")) phys->setContactTestBitmask(compConfig["contact_test_bitmask"].asInt());
				entity->addComponent(phys);
			}
			else if (compType == "PhysicsMovementComponent") {
				auto move = PhysicsMovementComponent::create();
				entity->addComponent(move);
			}
			else if (compType == "InputComponent") {
				auto input = InputComponent::create();
				entity->addComponent(input);
			}
		}
	}

	if (entity) {
		EntityCreatedEventData data(entity, entityType);
		EB->publish(EventType::ENTITY_CREATED, &data);
	}

	return entity;
}

_CEND