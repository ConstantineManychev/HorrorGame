#ifndef __VIEW_FACTORY_H__
#define __VIEW_FACTORY_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "CommonDefines.h"
#include "Basics/ServiceLocator.h"
#include <functional>
#include <unordered_map>

_CSTART

class ViewFactory
{
	friend class AppDelegate;
public:

	cocos2d::Node* createView(const std::string& aViewID);
	cocos2d::Node* createNodeFromValue(const cocos2d::ValueMap& aValueMap);

private:
	ViewFactory();

	using PropertyHandler = std::function<void(cocos2d::Node*, const cocos2d::Value&)>;

	std::unordered_map<std::string, PropertyHandler> mPropertyHandlers;

	void initPropertyHandlers();
	void applyProperties(cocos2d::Node* node, const cocos2d::ValueMap& params);

	cocos2d::FiniteTimeAction* createActionFromValue(const cocos2d::ValueMap& aValueMap);
};

#define VF SL->getService<GameSpace::ViewFactory>()

_CEND

#endif