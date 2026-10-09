#include "EditorScene.h"
#include "Managers/DataManager.h"
#include "Helpers/NodeHelper.h"

#include "ui/UIScrollView.h"
#include "ui/UIText.h"
#include "Managers/ScenesManager.h"
#include "Managers/ViewManager.h"
#include "Components/EditorMetadataComponent.h"

USING_NS_CC;
using namespace GameSpace;

const Color3B EditorScene::UIConfig::COLOR_PANEL_BG = Color3B(50, 50, 50);
const Color3B EditorScene::UIConfig::COLOR_TOOLBAR_BG = Color3B(40, 40, 40);
const Color4F EditorScene::UIConfig::COLOR_SELECTION = Color4F::GREEN;

BaseScene* EditorScene::createScene(const std::string& viewID)
{
	EditorScene* scene = static_cast<EditorScene*>(EditorScene::create());
	if (scene)
	{
		scene->setCurrentViewID(viewID);
		scene->setupEditorUI();

		return scene;
	}
	CC_SAFE_DELETE(scene);
	return nullptr;
}

bool EditorScene::init()
{
	if (!Scene::init())
	{
		return false;
	}

	mSelectionDrawNode = DrawNode::create();
	this->addChild(mSelectionDrawNode, UIConfig::TAG_SELECTION_BOX);
	mSelectionDrawNode->setTag(UIConfig::TAG_UI_LAYER);

	this->scheduleUpdate();

	return true;
}

void EditorScene::update(float delta)
{
	mSelectionDrawNode->clear();
	if (mSelectedNode && mSelectedNode->getParent())
	{
		Rect bbox = mSelectedNode->getBoundingBox();

		Vec2 bl = mSelectedNode->getParent()->convertToWorldSpace(bbox.origin);
		Vec2 tr = mSelectedNode->getParent()->convertToWorldSpace(bbox.origin + bbox.size);
		Vec2 br = Vec2(tr.x, bl.y);
		Vec2 tl = Vec2(bl.x, tr.y);

		Vec2 verts[] = { bl, br, tr, tl };
		mSelectionDrawNode->drawPolygon(verts, 4, Color4F(0, 1, 0, 0), 2, UIConfig::COLOR_SELECTION);

		updatePropertiesPanel();
	}
}

void EditorScene::useDefaultView()
{
}

void EditorScene::loadViewObjects(const std::string& viewID)
{
	mSelectedNode = nullptr;
	VM->changeView(viewID);
}

void EditorScene::saveViewObjects(const std::string& viewID)
{
	Node* viewRoot = nullptr;

	for (Node* child : this->getChildren())
	{
		if (child->getTag() == UIConfig::TAG_UI_LAYER) continue;
		if (child == mSelectionDrawNode) continue;
		if (child == mPropertiesLayout) continue;
		if (dynamic_cast<Camera*>(child)) continue;

		viewRoot = child;
		break;
	}

	if (!viewRoot)
	{
		return;
	}

	std::vector<sSceneObjectInfo> objects;
	auto visibleSize = Director::getInstance()->getVisibleSize();

	for (Node* child : viewRoot->getChildren())
	{
		sSceneObjectInfo info;
		info.name = child->getName();
		if (info.name.empty()) info.name = "Node_" + std::to_string(objects.size());

		auto metadata = dynamic_cast<EditorMetadataComponent*>(child->getComponent("EditorMetadata"));
		if (metadata)
		{
			info.type = metadata->type;
			info.textureFileName = metadata->textureFileName;
		}
		else
		{
			if (dynamic_cast<ui::Button*>(child)) info.type = "Button";
			else if (dynamic_cast<Sprite*>(child)) info.type = "Sprite";
			else info.type = "Node";
		}

		Size parentSize = viewRoot->getContentSize();
		if (parentSize.width == 0) parentSize = visibleSize;

		info.position.x = child->getPositionX() / parentSize.width;
		info.position.y = child->getPositionY() / parentSize.height;

		info.scaleX = child->getScaleX();
		info.scaleY = child->getScaleY();
		info.rotation = child->getRotation();
		info.zOrder = child->getLocalZOrder();

		objects.push_back(info);
	}

	DM->saveView(viewID, objects);
}

void EditorScene::setupEditorUI()
{
	mAvailableViews.clear();

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	std::vector<std::string> viewIDs = DM->getViewsIDs();

	for (const auto& viewID : viewIDs)
	{
		mAvailableViews.push_back({ viewID, viewID });
	}

	auto viewScrollView = ui::ScrollView::create();
	float scrollViewHeight = visibleSize.height - UIConfig::TOOLBAR_HEIGHT - (UIConfig::MARGIN * 2);
	viewScrollView->setContentSize(Size(UIConfig::PANEL_WIDTH, scrollViewHeight));
	viewScrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
	viewScrollView->setBounceEnabled(true);
	viewScrollView->setAnchorPoint(Vec2(0, 1));
	viewScrollView->setPosition(Vec2(UIConfig::MARGIN, visibleSize.height - UIConfig::TOOLBAR_HEIGHT - UIConfig::MARGIN));
	this->addChild(viewScrollView, UIConfig::TAG_UI_LAYER);
	viewScrollView->setTag(UIConfig::TAG_UI_LAYER);

	auto viewListLayout = ui::Layout::create();
	viewListLayout->setLayoutType(ui::Layout::Type::VERTICAL);
	viewListLayout->setContentSize(Size(UIConfig::PANEL_WIDTH, mAvailableViews.size() * UIConfig::PANEL_ITEM_HEIGHT));
	viewListLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
	viewListLayout->setBackGroundColor(UIConfig::COLOR_PANEL_BG);
	viewListLayout->setOpacity(200);
	viewScrollView->addChild(viewListLayout);
	viewScrollView->setInnerContainerSize(viewListLayout->getContentSize());
	viewListLayout->setTag(UIConfig::TAG_UI_LAYER);

	float currentY = viewListLayout->getContentSize().height - UIConfig::BUTTON_HEIGHT;
	for (const auto& viewPair : mAvailableViews)
	{
		auto viewButton = ui::Button::create();
		viewButton->setTitleText(viewPair.first);
		viewButton->setTitleFontSize(UIConfig::FONT_SIZE_NORMAL);
		viewButton->setContentSize(Size(UIConfig::BUTTON_WIDTH, UIConfig::BUTTON_HEIGHT));
		viewButton->ignoreContentAdaptWithSize(false);
		viewButton->setPosition(Vec2(viewListLayout->getContentSize().width / 2, currentY));
		viewButton->addTouchEventListener([&, viewID = viewPair.second](Ref* sender, ui::Widget::TouchEventType type){
			if (type == ui::Widget::TouchEventType::ENDED)
			{
				this->loadViewObjects(viewID);
				this->mCurrentViewID = viewID;
			}
		});
		viewListLayout->addChild(viewButton);
		currentY -= UIConfig::PANEL_ITEM_HEIGHT;
	}

	auto toolbar = ui::Layout::create();
	toolbar->setContentSize(Size(visibleSize.width, UIConfig::TOOLBAR_HEIGHT));
	toolbar->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
	toolbar->setBackGroundColor(UIConfig::COLOR_TOOLBAR_BG);
	toolbar->setPosition(Vec2(0, visibleSize.height - UIConfig::TOOLBAR_HEIGHT));
	toolbar->setAnchorPoint(Vec2(0, 0));
	this->addChild(toolbar, UIConfig::TAG_UI_LAYER);
	toolbar->setTag(UIConfig::TAG_UI_LAYER);

	float btnY = UIConfig::TOOLBAR_HEIGHT / 2;
	float btnStartX = 60.0f;
	float btnGap = 100.0f;

	auto addNodeButton = ui::Button::create();
	addNodeButton->setTitleText("Add Node");
	addNodeButton->setTitleFontSize(UIConfig::FONT_SIZE_BUTTON);
	addNodeButton->setPosition(Vec2(btnStartX, btnY));
	addNodeButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type) {
		if (type == ui::Widget::TouchEventType::ENDED) addNodeToScene("Node");
	});
	toolbar->addChild(addNodeButton);

	auto addSpriteButton = ui::Button::create();
	addSpriteButton->setTitleText("Add Sprite");
	addSpriteButton->setTitleFontSize(UIConfig::FONT_SIZE_BUTTON);
	addSpriteButton->setPosition(Vec2(btnStartX + btnGap, btnY));
	addSpriteButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type) {
		if (type == ui::Widget::TouchEventType::ENDED) addNodeToScene("Sprite", "HelloWorld.png");
	});
	toolbar->addChild(addSpriteButton);

	auto saveButton = ui::Button::create();
	saveButton->setTitleText("Save");
	saveButton->setTitleFontSize(UIConfig::FONT_SIZE_BUTTON);
	saveButton->setPosition(Vec2(visibleSize.width - 60, btnY));
	saveButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type) {
		if (type == ui::Widget::TouchEventType::ENDED) saveViewObjects(this->mCurrentViewID);
	});
	toolbar->addChild(saveButton);

	auto backButton = ui::Button::create();
	backButton->setTitleText("Play Game");
	backButton->setTitleFontSize(UIConfig::FONT_SIZE_BUTTON);
	backButton->setPosition(Vec2(visibleSize.width - 160, btnY));
	backButton->addTouchEventListener([&](Ref* aSendeer, ui::Widget::TouchEventType aType) {
		auto node = dynamic_cast<Node*>(aSendeer);
		if (aType == ui::Widget::TouchEventType::ENDED && node)
		{
			SM->openScene(node->getName());
		}
	});
	toolbar->addChild(backButton);

	setupPropertiesPanel();

	auto touchListener = EventListenerTouchOneByOne::create();
	touchListener->setSwallowTouches(true);
	touchListener->onTouchBegan = [&](Touch* touch, Event* event) {
		Vec2 touchLoc = touch->getLocation();

		bool hitUI = false;
		for (Node* child : this->getChildren())
		{
			if (child->getTag() == UIConfig::TAG_UI_LAYER && child->isVisible())
			{
				if (child->getBoundingBox().containsPoint(touchLoc))
				{
					hitUI = true;
					break;
				}
			}
		}
		if (hitUI) return false;

		Node* hitNode = nullptr;

		std::function<Node*(Node*)> findHit = [&](Node* root) -> Node* {
			auto& children = root->getChildren();
			for (auto it = children.rbegin(); it != children.rend(); ++it) {
				Node* child = *it;
				if (child->getTag() == UIConfig::TAG_UI_LAYER) continue;
				if (!child->isVisible()) continue;
				if (child == mSelectionDrawNode) continue;
				if (child == mPropertiesLayout) continue;
				if (dynamic_cast<Camera*>(child)) continue;

				Node* hit = findHit(child);
				if (hit) return hit;

				Rect bbox = child->getBoundingBox();
				Vec2 worldPos = child->getParent()->convertToWorldSpace(bbox.origin);
				Rect worldBox(worldPos.x, worldPos.y, bbox.size.width, bbox.size.height);

				if (worldBox.containsPoint(touchLoc)) {
					return child;
				}
			}
			return nullptr;
		};

		hitNode = findHit(this);

		mSelectedNode = hitNode;

		if (mSelectedNode && mSelectedNode->getParent())
		{
			cocos2d::Vec2 touchInParent = mSelectedNode->getParent()->convertToNodeSpace(touchLoc);
			mTouchOffset = mSelectedNode->getPosition() - touchInParent;
		}

		return true;
	};
	touchListener->onTouchMoved = [&](Touch* touch, Event* event) {
		if (mSelectedNode && mSelectedNode->getParent())
		{
			cocos2d::Vec2 touchLoc = touch->getLocation();
			cocos2d::Vec2 touchInParent = mSelectedNode->getParent()->convertToNodeSpace(touchLoc);
			mSelectedNode->setPosition(touchInParent + mTouchOffset);
		}
	};

	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

void EditorScene::setupPropertiesPanel()
{
	auto visibleSize = Director::getInstance()->getVisibleSize();

	mPropertiesLayout = ui::Layout::create();
	mPropertiesLayout->setContentSize(Size(UIConfig::PANEL_WIDTH, UIConfig::PROPERTIES_PANEL_HEIGHT));
	mPropertiesLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
	mPropertiesLayout->setBackGroundColor(UIConfig::COLOR_PANEL_BG);
	mPropertiesLayout->setOpacity(200);
	mPropertiesLayout->setAnchorPoint(Vec2(1, 0));
	mPropertiesLayout->setPosition(Vec2(visibleSize.width - UIConfig::MARGIN, UIConfig::MARGIN));
	this->addChild(mPropertiesLayout, UIConfig::TAG_UI_LAYER);
	mPropertiesLayout->setTag(UIConfig::TAG_UI_LAYER);

	float startY = UIConfig::PROPERTIES_PANEL_HEIGHT - 30.0f;
	float gapY = 30.0f;

	auto createField = [&](const std::string& title, int y) {
		auto label = ui::Text::create(title, "Arial", UIConfig::FONT_SIZE_NORMAL);
		label->setAnchorPoint(Vec2(0, 0.5));
		label->setPosition(Vec2(10, y));
		mPropertiesLayout->addChild(label);

		auto field = ui::TextField::create("0.00", "Arial", UIConfig::FONT_SIZE_NORMAL);
		field->setPosition(Vec2(100, y));
		field->setContentSize(Size(80, 20));
		field->setName(title);
		mPropertiesLayout->addChild(field);
		return field;
	};

	createField("Pos X", startY);
	createField("Pos Y", startY - gapY);
	createField("Scale X", startY - gapY * 2);
	createField("Scale Y", startY - gapY * 3);
	createField("Rotation", startY - gapY * 4);
}

void EditorScene::updatePropertiesPanel()
{
	if (!mSelectedNode || !mPropertiesLayout) return;

	auto setVal = [&](const std::string& name, float val) {
		auto field = dynamic_cast<ui::TextField*>(mPropertiesLayout->getChildByName(name));
		if (field && !field->isFocused()) {
			field->setString(StringUtils::format("%.2f", val));
		}
	};

	setVal("Pos X", mSelectedNode->getPositionX());
	setVal("Pos Y", mSelectedNode->getPositionY());
	setVal("Scale X", mSelectedNode->getScaleX());
	setVal("Scale Y", mSelectedNode->getScaleY());
	setVal("Rotation", mSelectedNode->getRotation());
}

void EditorScene::addNodeToScene(const std::string& type, const std::string& textureFileName)
{
	sSceneObjectInfo objInfo;
	objInfo.type = type;
	objInfo.name = type + std::to_string(this->getChildrenCount());
	objInfo.position = Director::getInstance()->getVisibleSize() / 2;
	objInfo.textureFileName = textureFileName;

	if (type == "Button" && !textureFileName.empty())
	{
		ValueMap customDataMap;
		customDataMap["textureFileName"] = Value(textureFileName);
		objInfo.customData = Value(customDataMap);
	}

	Node* newNode = NodeHelper::createNodeFromSceneObjectInfo(objInfo);
	if (newNode)
	{
		auto metadata = EditorMetadataComponent::create();
		metadata->type = type;
		metadata->textureFileName = textureFileName;
		newNode->addComponent(metadata);

		Node* viewRoot = nullptr;
		for (Node* child : this->getChildren()) {
			if (child->getTag() == UIConfig::TAG_UI_LAYER) continue;
			if (dynamic_cast<Camera*>(child)) continue;
			viewRoot = child;
			break;
		}
		if (viewRoot) viewRoot->addChild(newNode);
		else this->addChild(newNode);

		mSelectedNode = newNode;
	}
}

void EditorScene::updateCurrentNodeList(Node* aNode)
{
}

ui::Button* EditorScene::createButtonForChangeFolder(const std::string& aName, Node* aNode, float aCurrentY)
{
	return nullptr;
}