#include "ViewFactory.h"
#include "Helpers/NodeHelper.h"
#include "Managers/ViewManager.h" 
#include "Managers/DataManager.h"
#include "Managers/ScenesManager.h"
#include "Managers/GameDirector.h" 
#include "Basics/BaseLocation.h"
#include "ui/UIButton.h"
#include "Components/EditorMetadataComponent.h"
#include "Helpers/JsonHelper.h"
#include "EntityFactory.h"

USING_NS_CC;

namespace GameSpace {

	const std::unordered_set<std::string> ViewFactory::cExcludeParams = { "children", "actions", "creation", "prefab" };

	const std::unordered_map<std::string, ViewFactory::Params> ViewFactory::cParamTypeMap = {
		{"children", Params::CHILDREN}, {"params", Params::PARAMS}, {"actions", Params::ACTIONS}, {"prefab", Params::PREFAB},
		{"id", Params::ID}, {"res", Params::RES}, {"sprite_frame", Params::SPRITE_FRAME},
		{"res_normal", Params::RES_NORMAL},{"res_pressed", Params::RES_PRESSED},{"res_disable", Params::RES_DISABLE},
		{"layer", Params::LAYER}, {"opacity", Params::OPACITY},
		{"pos_x", Params::POS_X}, {"pos_y", Params::POS_Y},{"anch_x", Params::ANCH_X}, {"anch_y", Params::ANCH_Y},
		{"scale_x", Params::SCALE_X}, {"scale_y", Params::SCALE_Y}, {"rotation", Params::ROTATION}, {"is_visible", Params::IS_VISIBLE},
		{"color", Params::COLOR}
	};

	static const cocos2d::ValueMap MapNull;

	cocos2d::Node* ViewFactory::createNodeFromValue(const cocos2d::Value& aValue, cocos2d::Node* aParentNode)
	{
		cocos2d::Node* result = nullptr;

		if (aValue.getType() == cocos2d::Value::Type::MAP)
		{
			ValueMap valMap = aValue.asValueMap();

			if (valMap.find("prefab") != valMap.end())
			{
				std::string prefabPath = valMap.at("prefab").asString();
				Value prefabVal = JsonHelper::getValueFromJson(prefabPath);
				if (prefabVal.getType() == Value::Type::MAP)
				{
					ValueMap prefabMap = prefabVal.asValueMap();

					for (auto& kv : prefabMap) {
						if (kv.first == "params" && valMap.count("params")) {
							ValueMap& targetParams = valMap["params"].asValueMap();
							ValueMap sourceParams = kv.second.asValueMap();
							for (auto& pkv : sourceParams) {
								if (targetParams.find(pkv.first) == targetParams.end()) {
									targetParams[pkv.first] = pkv.second;
								}
							}
						}
						else if (valMap.find(kv.first) == valMap.end()) {
							valMap[kv.first] = kv.second;
						}
					}
				}
			}

			auto it = valMap.find("type");
			std::string typeName = "Node";
			if (it != valMap.end() && it->second.getType() == cocos2d::Value::Type::STRING)
			{
				typeName = it->second.asString();

				if (typeName == "Entity") {
					result = EF->createEntityFromConfig(valMap);
				}
				else {
					result = NodeHelper::createNodeForType(typeName);
				}

				if (result && typeName == "Player")
				{
					auto location = dynamic_cast<BaseLocation*>(aParentNode);
					if (location)
					{
						location->setupPlayer(result);
					}
				}
			}

			if (result)
			{
				auto metadata = EditorMetadataComponent::create();
				metadata->type = typeName;

				std::string textureName;
				if (valMap.count("res")) textureName = valMap.at("res").asString();
				else if (valMap.count("params")) {
					auto& p = valMap.at("params").asValueMap();
					if (p.count("res")) textureName = p.at("res").asString();
					else if (p.count("sprite_frame")) textureName = p.at("sprite_frame").asString();
				}
				metadata->textureFileName = textureName;
				result->addComponent(metadata);

				std::string paramName;
				auto& vmActions = VM->getActionsMap();

				for (auto it = valMap.begin(); it != valMap.end(); ++it)
				{
					paramName = it->first;
					if (cExcludeParams.find(paramName) == cExcludeParams.end())
					{
						fillNodeParamFromValue(result, paramName, it->second, vmActions, aParentNode);
					}
				}

				auto itChildren = valMap.find("children");
				if (itChildren != valMap.end())
				{
					fillNodeParamFromValue(result, itChildren->first, itChildren->second, vmActions, aParentNode);
				}

				auto itActions = valMap.find("actions");
				if (itActions != valMap.end())
				{
					parseActions(itActions->second, result, vmActions);
				}

				auto itParams = valMap.find("params");
				if (itParams != valMap.end())
				{
					fillNodeParamFromValue(result, itParams->first, itParams->second, vmActions, aParentNode);
				}

				if (!GD->isEditorMode())
				{
					VM->runActionForNode(result, "onCreate");
				}

				auto btn = dynamic_cast<cocos2d::ui::Button*>(result);
				if (btn)
				{
					if (GD->isEditorMode())
					{
						btn->setTouchEnabled(false);
					}
					else
					{
						btn->addTouchEventListener([result](cocos2d::Ref* sender, cocos2d::ui::Widget::TouchEventType type) {
							switch (type)
							{
							case cocos2d::ui::Widget::TouchEventType::BEGAN:
								VM->runActionForNode(result, "onBtnClickDown");
								break;
							case cocos2d::ui::Widget::TouchEventType::MOVED:
								VM->runActionForNode(result, "onBtnClickMove");
								break;
							case cocos2d::ui::Widget::TouchEventType::ENDED:
								VM->runActionForNode(result, "onBtnClickUp");
								break;
							case cocos2d::ui::Widget::TouchEventType::CANCELED:
								VM->runActionForNode(result, "onBtnClickCancel");
								break;
							default:
								break;
							}
						});
					}
				}
			}
		}

		return result;
	}

	void ViewFactory::fillNodeParamFromValue(cocos2d::Node* aNode, const std::string& aParamID, const cocos2d::Value& aValue, std::unordered_map<cocos2d::Node*, std::unordered_map<std::string, cocos2d::Vector<cocos2d::FiniteTimeAction*>>>& outActionsMap, cocos2d::Node* aParentNode)
	{
		if (!aNode) return;

		auto itParam = cParamTypeMap.find(aParamID);
		Params paramType = (itParam != cParamTypeMap.end()) ? itParam->second : Params::NONE;

		auto valueType = aValue.getType();

		switch (valueType)
		{
		case cocos2d::Value::Type::MAP:
		{
			const auto& valMap = aValue.asValueMap();
			for (auto it = valMap.begin(); it != valMap.end(); ++it)
			{
				switch (paramType)
				{
				case Params::CHILDREN:
				{
					cocos2d::Node* child = createNodeFromValue(it->second, aNode);
					if (child) {
						if (child->getName().empty()) child->setName(it->first);
						aNode->addChild(child);
					}
					break;
				}
				case Params::PARAMS:
					fillNodeParamFromValue(aNode, it->first, it->second, outActionsMap, aParentNode);
					break;
				case Params::ACTIONS:
					parseActions(it->second, aNode, outActionsMap);
					break;
				default:
					break;
				}
			}
			break;
		}
		case cocos2d::Value::Type::VECTOR:
		{
			const auto& valVector = aValue.asValueVector();
			for (auto& val : valVector)
			{
				switch (paramType)
				{
				case Params::CHILDREN:
					aNode->addChild(createNodeFromValue(val, aNode));
					break;
				case Params::ACTIONS:
					parseActions(val, aNode, outActionsMap);
					break;
				default:
					break;
				}
			}
			break;
		}
		case cocos2d::Value::Type::STRING:
		{
			switch (paramType)
			{
			case Params::ID:
				aNode->setName(aValue.asString());
				break;
			case Params::RES:
			{
				auto sprite = dynamic_cast<cocos2d::Sprite*>(aNode);
				if (sprite) sprite->initWithFile(aValue.asString());
				break;
			}
			case Params::SPRITE_FRAME:
			{
				auto sprite = dynamic_cast<cocos2d::Sprite*>(aNode);
				if (sprite)
				{
					std::string frameName = aValue.asString();
					auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(frameName);
					if (frame)
					{
						sprite->setSpriteFrame(frame);
					}
					else
					{
						CCLOG("ViewFactory ERROR: SpriteFrame '%s' not found. Check .plist loaded or name in json.", frameName.c_str());
					}
				}
				break;
			}
			case Params::RES_NORMAL:
			{
				auto btn = dynamic_cast<cocos2d::ui::Button*>(aNode);
				if (btn) btn->loadTextureNormal(aValue.asString());
				break;
			}
			case Params::RES_PRESSED:
			{
				auto btn = dynamic_cast<cocos2d::ui::Button*>(aNode);
				if (btn) btn->loadTexturePressed(aValue.asString());
				break;
			}
			case Params::RES_DISABLE:
			{
				auto btn = dynamic_cast<cocos2d::ui::Button*>(aNode);
				if (btn) btn->loadTextureDisabled(aValue.asString());
				break;
			}
			case Params::COLOR:
				aNode->setColor(JsonHelper::toColor3B(aValue));
				break;
			}
			break;
		}
		case cocos2d::Value::Type::BOOLEAN:
		{
			if (paramType == Params::IS_VISIBLE)
			{
				aNode->setVisible(aValue.asBool());
			}
			break;
		}
		case cocos2d::Value::Type::INTEGER:
		{
			if (paramType == Params::LAYER) aNode->setLocalZOrder(aValue.asInt());
			else if (paramType == Params::OPACITY) aNode->setOpacity(aValue.asInt());
			break;
		}
		case cocos2d::Value::Type::FLOAT:
		case cocos2d::Value::Type::DOUBLE:
		{
			cocos2d::Size parentSize = cocos2d::Size::ZERO;

			cocos2d::Node* parent = aParentNode;
			if (!parent)
			{
				parent = aNode->getParent();
			}

			if (parent)
			{
				parentSize = parent->getContentSize();
			}
			else
			{
				parentSize = cocos2d::Director::getInstance()->getWinSize();
			}

			float val = aValue.asFloat();

			switch (paramType)
			{
			case Params::POS_X: aNode->setPositionX(parentSize.width * val); break;
			case Params::POS_Y: aNode->setPositionY(parentSize.height * val); break;
			case Params::SCALE_X: aNode->setScaleX(val); break;
			case Params::SCALE_Y: aNode->setScaleY(val); break;
			case Params::ROTATION: aNode->setRotation(val); break;
			case Params::ANCH_X:
			{
				auto anch = aNode->getAnchorPoint();
				anch.x = val;
				aNode->setAnchorPoint(anch);
				break;
			}
			case Params::ANCH_Y:
			{
				auto anch = aNode->getAnchorPoint();
				anch.y = val;
				aNode->setAnchorPoint(anch);
				break;
			}
			}
			break;
		}
		default:
			break;
		}
	}

	void ViewFactory::parseActions(const cocos2d::Value& aValue, cocos2d::Node* aNode, std::unordered_map<cocos2d::Node*, std::unordered_map<std::string, cocos2d::Vector<cocos2d::FiniteTimeAction*>>>& outActionsMap)
	{
		if (aValue.getType() == cocos2d::Value::Type::MAP)
		{
			const auto& valMap = aValue.asValueMap();
			for (auto it = valMap.begin(); it != valMap.end(); ++it)
			{
				if (it->second.getType() == cocos2d::Value::Type::VECTOR)
				{
					std::string actionVecName = it->first;
					const auto& actionsVec = it->second.asValueVector();

					for (auto& action : actionsVec)
					{
						cocos2d::FiniteTimeAction* result = createActionFromValue(action, aNode);
						if (aNode && !actionVecName.empty() && result)
						{
							outActionsMap[aNode][actionVecName].pushBack(result);
						}
					}
				}
			}
		}
	}

	cocos2d::FiniteTimeAction* ViewFactory::createActionFromValue(const cocos2d::Value& aValue, cocos2d::Node* aNode)
	{
		cocos2d::FiniteTimeAction* result = nullptr;

		if (aValue.getType() == cocos2d::Value::Type::MAP)
		{
			const auto& actionMap = aValue.asValueMap();

			auto it = actionMap.find("runAction");
			if (it != actionMap.end() && it->second.getType() == cocos2d::Value::Type::STRING)
			{
				std::string actionName = it->second.asString();

				auto getFloat = [&](const std::string& key) {
					return (actionMap.find(key) != actionMap.end()) ? actionMap.at(key).asFloat() : 0.0f;
				};

				if (actionName == "fade_in")
				{
					result = cocos2d::FadeIn::create(getFloat("duration"));
				}
				else if (actionName == "fade_out")
				{
					result = cocos2d::FadeOut::create(getFloat("duration"));
				}
				else if (actionName == "delay_time")
				{
					result = cocos2d::DelayTime::create(getFloat("duration"));
				}
				else if (actionName == "change_view")
				{
					std::string viewID = (actionMap.find("id") != actionMap.end()) ? actionMap.at("id").asString() : "";
					result = cocos2d::CallFunc::create([viewID]()
					{
						if (!GD->isEditorMode())
						{
							VM->changeView(viewID);
						}
					});
				}
				else if (actionName == "forget_button")
				{
					auto btn = dynamic_cast<cocos2d::ui::Button*>(aNode);
					if (btn && actionMap.count("conditions"))
					{
						ValueMap conditions = actionMap.at("conditions").asValueMap();
						result = cocos2d::CallFunc::create([conditions, btn]()
						{
							bool isAllConditions = isAllConditionsMeetRequirements(conditions, btn);
							if (isAllConditions)
							{
							}
						});
					}
				}
			}
		}
		return result;
	}

	bool ViewFactory::isAllConditionsMeetRequirements(const cocos2d::ValueMap& aMap, cocos2d::Node* aNode)
	{
		bool result = true;
		for (auto it = aMap.begin(); it != aMap.end(); ++it)
		{
			if (it->second.getType() == cocos2d::Value::Type::BOOLEAN)
			{
				bool value = it->second.asBool();
				if (it->first == "is_Highlighted")
				{
					auto btn = dynamic_cast<cocos2d::ui::Button*>(aNode);
					if (btn) result = (btn->isHighlighted() == value);
					else result = false;
				}
			}
			else result = false;

			if (!result) break;
		}
		return result;
	}

}