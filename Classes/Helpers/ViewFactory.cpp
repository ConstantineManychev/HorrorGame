#include "ViewFactory.h"
#include "NodeHelper.h"
#include "Managers/ViewManager.h" // Для вызова VM->changeView в CallFunc
#include "Managers/DataManager.h"
#include "Managers/ScenesManager.h"
#include "Basics/BaseLocation.h"
#include "ui/UIButton.h"

_CSTART

const std::unordered_set<std::string> ViewFactory::cExcludeParams = { "children", "actions", "creation" };

const std::unordered_map<std::string, ViewFactory::Params> ViewFactory::cParamTypeMap = {
	{"children", Params::CHILDREN}, {"params", Params::PARAMS}, {"actions", Params::ACTIONS},
	{"id", Params::ID}, {"res", Params::RES},
	{"res_normal", Params::RES_NORMAL},{"res_pressed", Params::RES_PRESSED},{"res_disable", Params::RES_DISABLE},
	{"layer", Params::LAYER}, {"opacity", Params::OPACITY},
	{"pos_x", Params::POS_X}, {"pos_y", Params::POS_Y},{"anch_x", Params::ANCH_X}, {"anch_y", Params::ANCH_Y}
};

static const BValueMap MapNull;

Node* ViewFactory::createNodeFromBValue(const BValue& aBValue, Node* aParentNode)
{
	// Так как ViewFactory статический, нам нужно временное хранилище для actions, 
	// которое мы потом передадим в ViewManager, или ViewManager должен предоставлять метод регистрации.
	// В данном архитектурном решении, ViewFactory создает ноды, но действия регистрируются через ViewManager
	// Однако, рекурсивный парсинг требует прокидывания контекста.
	// Упрощение: Мы используем ViewManager::getInstance() для регистрации действий напрямую в process.
	
	Node* result = nullptr;

	if (aBValue.isMap())
	{
		const auto& valMap = aBValue.getValueMap();

		auto it = valMap.find("type");
		if (it != valMap.end() && it->second.isString())
		{
			auto typeName = it->second.getString();
			result = NodeHelper::createNodeForType(typeName);

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
			// Настройка свойств
			std::string paramName;
			
			// Нам нужно собрать actions для этого конкретного узла и его детей
			// В текущей архитектуре actions хранятся в ViewManager. 
			// Мы будем регистрировать их там.
			
			auto& vmActions = VM->getActionsMap(); // Friendship or public accessor needed

			for (auto it = valMap.begin(); it != valMap.end(); ++it)
			{
				paramName = it->first;
				if (cExcludeParams.find(paramName) == cExcludeParams.end())
				{
					fillNodeParamFromBValue(result, paramName, it->second, vmActions, aParentNode);
				}
			}

			auto itChildren = valMap.find("children");
			if (itChildren != valMap.end())
			{
				fillNodeParamFromBValue(result, itChildren->first, itChildren->second, vmActions, aParentNode);
			}

			auto itActions = valMap.find("actions");
			if (itActions != valMap.end())
			{
				// Специальная обработка для actions
				parseActions(itActions->second, result, vmActions);
			}
			
			// Setup Listeners
			if (SM->getCurrentSceneID() != "editor")
			{
				VM->runActionForNode(result, "onCreate");
			}

			auto btn = dynamic_cast<ui::Button*>(result);
			if (btn)
			{
				btn->addTouchEventListener([result](Ref* sender, ui::Widget::TouchEventType type) {
					switch (type)
					{
					case ui::Widget::TouchEventType::BEGAN:
						VM->runActionForNode(result, "onBtnClickDown");
						break;
					case ui::Widget::TouchEventType::MOVED:
						VM->runActionForNode(result, "onBtnClickMove");
						break;
					case ui::Widget::TouchEventType::ENDED:
						VM->runActionForNode(result, "onBtnClickUp");
						break;
					case ui::Widget::TouchEventType::CANCELED:
						VM->runActionForNode(result, "onBtnClickCancel");
						break;
					default:
						break;
					}
				});
			}
		}
	}

	return result;
}

void ViewFactory::fillNodeParamFromBValue(Node* aNode, const std::string& aParamID, const BValue& aBValue, std::unordered_map<Node*, std::unordered_map<std::string, Vector<FiniteTimeAction*>>>& outActionsMap, Node* aParentNode)
{
	if (!aNode) return;

	auto itParam = cParamTypeMap.find(aParamID);
	Params paramType = (itParam != cParamTypeMap.end()) ? itParam->second : Params::NONE;

	auto valueType = aBValue.getType();

	switch (valueType)
	{
	case BValue::Type::MAP:
	{
		const auto& valMap = aBValue.getValueMap();
		for (auto it = valMap.begin(); it != valMap.end(); ++it)
		{
			switch (paramType)
			{
			case Params::CHILDREN:
				// Pass 'aNode' as parent for children
				aNode->addChild(createNodeFromBValue(it->second, aNode));
				break;
			case Params::PARAMS:
				fillNodeParamFromBValue(aNode, it->first, it->second, outActionsMap, aParentNode);
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
	case BValue::Type::VECTOR:
	{
		const auto& valVector = aBValue.getValueVector();
		for (auto& val : valVector)
		{
			switch (paramType)
			{
			case Params::CHILDREN:
				aNode->addChild(createNodeFromBValue(val, aNode));
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
	case BValue::Type::STRING:
	{
		switch (paramType)
		{
		case Params::ID:
			aNode->setName(aBValue.getString());
			break;
		case Params::RES:
		{
			auto sprite = dynamic_cast<Sprite*>(aNode);
			if (sprite) sprite->initWithFile(aBValue.getString());
			break;
		}
		case Params::RES_NORMAL:
		{
			auto btn = dynamic_cast<ui::Button*>(aNode);
			if (btn) btn->loadTextureNormal(aBValue.getString());
			break;
		}
		case Params::RES_PRESSED:
		{
			auto btn = dynamic_cast<ui::Button*>(aNode);
			if (btn) btn->loadTexturePressed(aBValue.getString());
			break;
		}
		case Params::RES_DISABLE:
		{
			auto btn = dynamic_cast<ui::Button*>(aNode);
			if (btn) btn->loadTextureDisabled(aBValue.getString());
			break;
		}
		}
		break;
	}
	case BValue::Type::BOOLEAN:
	{
		if (paramType == Params::IS_VISIBLE)
		{
			aNode->setVisible(aBValue.getBool());
		}
		break;
	}
	case BValue::Type::INTEGER:
	{
		if (paramType == Params::LAYER) aNode->setLocalZOrder(aBValue.getInt());
		else if (paramType == Params::OPACITY) aNode->setOpacity(aBValue.getInt());
		break;
	}
	case BValue::Type::FLOAT:
	case BValue::Type::DOUBLE:
	{
		Size parentSize = Size::ZERO;
		
		// Use explicit parent if provided, otherwise check actual parent (which is likely null during creation)
		Node* parent = aParentNode;
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
			parentSize = Director::getInstance()->getWinSize();
		}

		float val = aBValue.getFloat();

		switch (paramType)
		{
		case Params::POS_X: aNode->setPositionX(parentSize.width * val); break;
		case Params::POS_Y: aNode->setPositionY(parentSize.height * val); break;
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
	case BValue::Type::COLOR3B:
		aNode->setColor(aBValue.getColor3B());
		break;
	case BValue::Type::SIZE:
		aNode->setContentSize(aBValue.getSize());
		break;
	default:
		break;
	}
}

void ViewFactory::parseActions(const BValue& aBValue, Node* aNode, std::unordered_map<Node*, std::unordered_map<std::string, Vector<FiniteTimeAction*>>>& outActionsMap)
{
	if (aBValue.isMap())
	{
		const auto& valMap = aBValue.getValueMap();
		for (auto it = valMap.begin(); it != valMap.end(); ++it)
		{
			if (it->second.isVector())
			{
				std::string actionVecName = it->first;
				const auto& actionsVec = it->second.getValueVector();

				for (auto action : actionsVec)
				{
					FiniteTimeAction* result = createActionFromBValue(action, aNode);
					if (aNode && !actionVecName.empty() && result)
					{
						outActionsMap[aNode][actionVecName].pushBack(result);
						// Retain action logic handled by Vector implicitly if copied? 
						// No, Vector holds Ref*, need to be careful. 
						// Cocos Vector retains added objects.
						// But we are filling the map that lives in ViewManager.
					}
				}
			}
		}
	}
}

FiniteTimeAction* ViewFactory::createActionFromBValue(const BValue& aBValue, Node* aNode)
{
	FiniteTimeAction* result = nullptr;

	if (aBValue.isMap())
	{
		const auto& actionMap = aBValue.getValueMap();
		auto it = actionMap.find("runAction");
		if (it != actionMap.end() && it->second.isString())
		{
			std::string actionName = it->second.getString();
			if (actionName == "fade_in")
			{
				result = FadeIn::create(getParamFloat(actionMap, "duration"));
			}
			else if (actionName == "fade_out")
			{
				result = FadeOut::create(getParamFloat(actionMap, "duration"));
			}
			else if (actionName == "delay_time")
			{
				result = DelayTime::create(getParamFloat(actionMap, "duration"));
			}
			else if (actionName == "change_view")
			{
				auto viewID = getParamString(actionMap, "id");
				result = CallFunc::create([viewID]()
				{
					if (SM->getCurrentSceneID() != "editor")
					{
						VM->changeView(viewID);
					}
				});
			}
			else if (actionName == "forget_button")
			{
				auto btn = dynamic_cast<ui::Button*>(aNode);
				if (btn)
				{
					result = CallFunc::create([actionMap, btn]()
					{
						auto conditions = getParamMap(actionMap, "conditions");
						bool isAllConditions = isAllConditionsMeetRequirements(conditions, btn);

						if (isAllConditions)
						{
							// Logic here
						}
					});
				}
			}
		}
	}
	return result;
}

bool ViewFactory::isAllConditionsMeetRequirements(const BValueMap& aMap, Node* aNode)
{
	bool result = true;
	for (auto it = aMap.begin(); it != aMap.end(); ++it)
	{
		if (it->second.isBoolean())
		{
			bool value = it->second.getBool();
			if (it->first == "is_Highlighted")
			{
				auto btn = dynamic_cast<ui::Button*>(aNode);
				if (btn) result = (btn->isHighlighted() == value);
				else result = false;
			}
		}
		else result = false;
		
		if (!result) break;
	}
	return result;
}

const BValueMap& ViewFactory::getParamMap(const BValueMap& aMap, const std::string& aParam)
{
	auto it = aMap.find(aParam);
	if (it != aMap.end() && it->second.isMap()) return it->second.getValueMap();
	return MapNull;
}

std::string ViewFactory::getParamString(const BValueMap& aMap, const std::string& aParam)
{
	auto it = aMap.find(aParam);
	if (it != aMap.end() && it->second.isString()) return it->second.getString();
	return "";
}

float ViewFactory::getParamFloat(const BValueMap& aMap, const std::string& aParam)
{
	auto it = aMap.find(aParam);
	if (it != aMap.end() && (it->second.isFloat() || it->second.isDouble())) return it->second.getFloat();
	return 0.f;
}

bool ViewFactory::getParamBool(const BValueMap& aMap, const std::string& aParam)
{
	auto it = aMap.find(aParam);
	if (it != aMap.end() && it->second.isBoolean()) return it->second.getBool();
	return false;
}

_CEND
