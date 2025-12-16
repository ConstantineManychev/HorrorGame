#include "ViewFactory.h"
#include "Helpers/JsonHelper.h"
#include "Managers/DataManager.h"
#include "Managers/ViewManager.h"
#include "Managers/GameDirector.h"
#include "Managers/CutsceneManager.h"
#include "EntityFactory.h"

USING_NS_CC;
using namespace cocos2d::ui;

_CSTART

ViewFactory::ViewFactory()
{
	initPropertyHandlers();
}

void ViewFactory::initPropertyHandlers()
{
	mPropertyHandlers["pos_x"] = [](Node* n, const Value& v) { n->setPositionX(v.asFloat()); };
	mPropertyHandlers["pos_y"] = [](Node* n, const Value& v) { n->setPositionY(v.asFloat()); };
	mPropertyHandlers["anch_x"] = [](Node* n, const Value& v) { n->setAnchorPoint(Vec2(v.asFloat(), n->getAnchorPoint().y)); };
	mPropertyHandlers["anch_y"] = [](Node* n, const Value& v) { n->setAnchorPoint(Vec2(n->getAnchorPoint().x, v.asFloat())); };
	mPropertyHandlers["position"] = [](Node* n, const Value& v) { n->setPosition(JsonHelper::toVec2(v)); };
	mPropertyHandlers["layer"] = [](Node* n, const Value& v) { n->setLocalZOrder(v.asInt()); };

	mPropertyHandlers["content_size"] = [](Node* n, const Value& v) { n->setContentSize(JsonHelper::toSize(v)); };
	mPropertyHandlers["scale_x"] = [](Node* n, const Value& v) { n->setScaleX(v.asFloat()); };
	mPropertyHandlers["scale_y"] = [](Node* n, const Value& v) { n->setScaleY(v.asFloat()); };
	mPropertyHandlers["rotation"] = [](Node* n, const Value& v) { n->setRotation(v.asFloat()); };

	mPropertyHandlers["visible"] = [](Node* n, const Value& v) { n->setVisible(v.asBool()); };
	mPropertyHandlers["opacity"] = [](Node* n, const Value& v) { n->setOpacity(v.asByte()); };
	mPropertyHandlers["color"] = [](Node* n, const Value& v) { n->setColor(JsonHelper::toColor3B(v)); };

	mPropertyHandlers["res"] = [](Node* n, const Value& v) {
		if (auto s = dynamic_cast<Sprite*>(n)) {
			s->setTexture(v.asString());
			if (s->getTexture()) s->getTexture()->setAliasTexParameters();
		}
		else if (auto b = dynamic_cast<Button*>(n)) {
			b->loadTextureNormal(v.asString());
		}
	};

	mPropertyHandlers["res_normal"] = [](Node* n, const Value& v) {
		if (auto b = dynamic_cast<Button*>(n)) {
			b->loadTextureNormal(v.asString());
		}
	};

	mPropertyHandlers["sprite_frame"] = [](Node* n, const Value& v) {
		if (auto s = dynamic_cast<Sprite*>(n)) {
			s->setSpriteFrame(v.asString());
			if (s->getTexture()) s->getTexture()->setAliasTexParameters();
		}
	};

	mPropertyHandlers["res_pressed"] = [](Node* n, const Value& v) {
		if (auto b = dynamic_cast<Button*>(n)) b->loadTexturePressed(v.asString());
	};

	mPropertyHandlers["text"] = [](Node* n, const Value& v) {
		if (auto l = dynamic_cast<Label*>(n)) l->setString(v.asString());
		else if (auto b = dynamic_cast<Button*>(n)) b->setTitleText(v.asString());
	};

	mPropertyHandlers["font_size"] = [](Node* n, const Value& v) {
		if (auto b = dynamic_cast<Button*>(n)) b->setTitleFontSize(v.asFloat());
	};
}

void ViewFactory::applyProperties(Node* node, const ValueMap& params)
{
	for (const auto& pair : params)
	{
		auto it = mPropertyHandlers.find(pair.first);
		if (it != mPropertyHandlers.end())
		{
			it->second(node, pair.second);
		}
	}
}

Node* ViewFactory::createView(const std::string& aViewID)
{
	ValueMap info = DM->getViewInfoByID(aViewID);
	if (info.empty()) return nullptr;

	Node* view = createNodeFromValue(info);

	if (view && info.count("params"))
	{
		view->setScaleX(DM->getScaleY());
		view->setScaleY(DM->getScaleY());

		ValueMap params = info.at("params").asValueMap();

		if (params.count("pos_x") && params.count("pos_y"))
		{
			Size screenSize = Director::getInstance()->getVisibleSize();
			float px = params.at("pos_x").asFloat();
			float py = params.at("pos_y").asFloat();

			view->setPosition(screenSize.width * px, screenSize.height * py);
		}
	}

	return view;
}

Node* ViewFactory::createNodeFromValue(const ValueMap& aValueMap)
{
	if (aValueMap.count("prefab"))
	{
		std::string prefabPath = aValueMap.at("prefab").asString();
		Value prefabVal = JsonHelper::getValueFromJson(prefabPath);

		if (prefabVal.getType() == Value::Type::MAP)
		{
			ValueMap mergedMap = prefabVal.asValueMap();

			for (const auto& pair : aValueMap)
			{
				const std::string& key = pair.first;
				if (key == "prefab") continue; 

				if (key == "params" && mergedMap.count("params"))
				{
					ValueMap& targetParams = mergedMap["params"].asValueMap();
					ValueMap sourceParams = pair.second.asValueMap();
					for (const auto& p : sourceParams)
					{
						targetParams[p.first] = p.second;
					}
				}
				else
				{
					mergedMap[key] = pair.second;
				}
			}

			return createNodeFromValue(mergedMap);
		}
	}

	Node* result = nullptr;
	std::string type = aValueMap.count("type") ? aValueMap.at("type").asString() : "Node";

	if (type == "Sprite")
	{
		result = Sprite::create();
	}
	else if (type == "Button")
	{
		result = Button::create();
	}
	else if (type == "Label")
	{
		result = Label::createWithTTF("", "fonts/arial.ttf", 24);
	}
	else if (type == "Entity")
	{
		result = EF->createEntityFromConfig(aValueMap);
	}
	else
	{
		result = Node::create();
	}

	if (!result) return nullptr;

	if (aValueMap.count("id")) {
		result->setName(aValueMap.at("id").asString());
	}

	if (aValueMap.count("params"))
	{
		ValueMap params = aValueMap.at("params").asValueMap();
		if (params.count("content_size")) {
			result->setContentSize(JsonHelper::toSize(params.at("content_size")));
		}

		applyProperties(result, params);
	}

	if (aValueMap.count("children"))
	{
		ValueMap children = aValueMap.at("children").asValueMap();
		for (auto& childPair : children)
		{
			Node* child = createNodeFromValue(childPair.second.asValueMap());
			if (child) {
				child->setName(childPair.first);

				if (childPair.second.asValueMap().count("params")) {
					ValueMap childParams = childPair.second.asValueMap().at("params").asValueMap();
					float px = childParams.count("pos_x") ? childParams.at("pos_x").asFloat() : 0.0f;
					float py = childParams.count("pos_y") ? childParams.at("pos_y").asFloat() : 0.0f;
					child->setPosition(result->getContentSize().width * px, result->getContentSize().height * py);
				}

				result->addChild(child);
			}
			if (child->getName().empty())
			{
				child->setName(childPair.first);
			}
		}
	}

	if (aValueMap.count("actions")) {
		if (GD && !GD->isEditorMode()) {
			ValueMap actionsMap = aValueMap.at("actions").asValueMap();

			if (actionsMap.count("onCreate")) {
				ValueVector onCreateList = actionsMap.at("onCreate").asValueVector();
				for (const auto& actionVal : onCreateList) {
					auto action = createActionFromValue(actionVal.asValueMap());
					if (action) {
						result->runAction(action);
					}
				}
			}

			if (auto btn = dynamic_cast<ui::Button*>(result)) {
				if (actionsMap.count("onBtnClickUp")) {
					ValueVector clickActions = actionsMap.at("onBtnClickUp").asValueVector();

					btn->addTouchEventListener([this, clickActions](Ref* sender, ui::Widget::TouchEventType type) {
						if (type == ui::Widget::TouchEventType::ENDED) {
							Node* target = dynamic_cast<Node*>(sender);
							for (const auto& actVal : clickActions) {
								auto action = createActionFromValue(actVal.asValueMap());
								if (action && target) target->runAction(action);
							}
						}
					});
				}
			}
		}
	}

	return result;
}

FiniteTimeAction* ViewFactory::createActionFromValue(const ValueMap& aValueMap)
{
	if (aValueMap.count("runAction")) {
		std::string actionName = aValueMap.at("runAction").asString();
		if (actionName == "play_cutscene" && aValueMap.count("path")) {
			std::string path = aValueMap.at("path").asString();
			return CallFunc::create([path]() { CSM->playCutscene(path); });
		}
		if (actionName == "change_view" && aValueMap.count("id")) {
			std::string id = aValueMap.at("id").asString();
			return CallFunc::create([id]() { VM->changeView(id); });
		}
	}
	return nullptr;
}

_CEND