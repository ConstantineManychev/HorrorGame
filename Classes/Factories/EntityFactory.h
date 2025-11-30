#ifndef __ENTITY_FACTORY_H__
#define __ENTITY_FACTORY_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include <functional>
#include <map>
#include <string>

_CSTART

class EntityFactory
{
public:
	static EntityFactory* getInstance();

	using EntityCreator = std::function<cocos2d::Node*()>;

	void registerEntity(const std::string& type, EntityCreator creator);

	cocos2d::Node* createEntity(const std::string& type);

	cocos2d::Node* createEntityFromConfig(const cocos2d::ValueMap& config);

private:
	EntityFactory();
	std::map<std::string, EntityCreator> mCreators;
};

#define EF EntityFactory::getInstance()

_CEND

#endif