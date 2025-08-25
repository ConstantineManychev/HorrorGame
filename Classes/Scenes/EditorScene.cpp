#include "EditorScene.h"
#include "Managers/DataManager.h"
#include "Helpers/NodeHelper.h"
#include "ui/UIButton.h"
#include "ui/UILayout.h"
#include "ui/UIScrollView.h"
#include "Managers/ScenesManager.h" // Required for SM macro

_USEC

Scene* EditorScene::createScene(const std::string& viewID)
{
    EditorScene* scene = static_cast<EditorScene*>(EditorScene::create());
    if (scene)
    {
		scene->mCurrentViewID = viewID; // Set the current view ID
		scene->setupEditorUI(); // Setup UI after view ID is set
		scene->loadViewObjects(scene->mCurrentViewID.empty() ? "main" : scene->mCurrentViewID); // Load view after UI is setup
		
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

	// UI setup and scene loading will be handled in createScene to ensure correct order
	//setupEditorUI(); // Removed from init

    return true;
}

void EditorScene::loadViewObjects(const std::string& viewID)
{
	CCLOG("EditorScene: Loading view from: %s", viewID.c_str());

	// Remove all existing game nodes (keep UI elements)
	Vector<Node*> nodesToRemove;
	for (Node* child : this->getChildren())
	{
		if (child->getTag() != 9999) // Assuming editor UI elements have a specific tag
		{
			nodesToRemove.pushBack(child);
		}
	}
	for (Node* node : nodesToRemove)
	{
		this->removeChild(node, true);
	}

	// Get view info from DataManager
	const BValue& viewInfo = DM->getViewInfoByID(viewID);

	if (viewInfo.isMap() && viewInfo.getValueMap().count("children"))
	{
		const BValue& childrenValue = viewInfo.getValueMap().at("children");

		if (childrenValue.isVector())
		{
			const auto& objects = childrenValue.getValueVector();
			CCLOG("EditorScene: Loaded %zu objects for view: %s", objects.size(), viewID.c_str());

			for (const auto& objBValue : objects)
			{
				sSceneObjectInfo objInfo;
				DM->parseSceneObject(objBValue.getValueMap(), objInfo); // Assuming parseSceneObject takes BValueMap
				Node* node = NodeHelper::createNodeFromSceneObjectInfo(objInfo);
				if (node)
				{
					this->addChild(node, node->getLocalZOrder(), node->getName());
				}
			}
		}
	}
	else
	{
		CCLOG("EditorScene: Failed to load view info for %s or no children found.", viewID.c_str());
	}
}

void EditorScene::saveViewObjects(const std::string& viewID)
{
	CCLOG("EditorScene: Saving view to: %s", viewID.c_str());

	std::vector<sSceneObjectInfo> sceneObjects;

	// Iterate through all children to collect their info
	for (Node* child : this->getChildren())
	{
		// Skip editor UI elements
		if (child->getTag() == 9999) // Assuming editor UI elements have a specific tag
		{
			continue;
		}

		sSceneObjectInfo objInfo;
		objInfo.name = child->getName().empty() ? "unnamed_node" : child->getName();

		if (dynamic_cast<Sprite*>(child))
		{
			objInfo.type = "Sprite";
			Sprite* sprite = static_cast<Sprite*>(child);
			if (sprite->getTexture())
			{
				objInfo.textureFileName = sprite->getTexture()->getPath();
			}
		}
		else if (dynamic_cast<ui::Button*>(child))
		{
			objInfo.type = "Button";
			ui::Button* button = static_cast<ui::Button*>(child);
			if (objInfo.customData.isMap())
			{
				auto it = objInfo.customData.getValueMap().find("textureFileName");
				if (it != objInfo.customData.getValueMap().end() && it->second.isString())
				{
					objInfo.textureFileName = it->second.getString();
				}
			}
			else
			{
				objInfo.textureFileName = "HelloWorld.png"; 
			}

		}
		else
		{
			objInfo.type = "Node";
		}

		objInfo.position = child->getPosition();
		objInfo.scaleX = child->getScaleX();
		objInfo.scaleY = child->getScaleY();
		objInfo.zOrder = child->getLocalZOrder(); 

		if (child->getUserObject())
		{
			if (objInfo.type == "Button")
			{
				
				if (objInfo.customData.isMap())
				{
					auto it = objInfo.customData.getValueMap().find("textureFileName");
					if (it != objInfo.customData.getValueMap().end() && it->second.isString())
					{
						objInfo.textureFileName = it->second.getString();
					}
				}
				else
				{
					objInfo.textureFileName = "HelloWorld.png"; 
				}
			}
		}

		sceneObjects.push_back(objInfo);
	}

	// Save to the original view config file, not a separate scene file
	DM->saveView(viewID, sceneObjects); // This will require a new DataManager::saveView method
}

void EditorScene::setupEditorUI()
{
	// Clear previous view list to avoid duplicates
	mAvailableViews.clear();

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	// --- View Selection UI ---
	// Load available view IDs from DataManager
	std::vector<std::string> viewIDs = DM->getViewsIDs();

	for (const auto& viewID : viewIDs)
	{
		mAvailableViews.push_back({viewID, viewID}); // Store view ID as both name and ID
	}

	// Create a scroll view for view selection
	auto viewScrollView = ui::ScrollView::create();
	viewScrollView->setContentSize(Size(200, visibleSize.height - 100));
	viewScrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
	viewScrollView->setBounceEnabled(true);
	viewScrollView->setAnchorPoint(Vec2(0, 1));
	viewScrollView->setPosition(Vec2(10, visibleSize.height - 10));
	this->addChild(viewScrollView, 9999);
	viewScrollView->setTag(9999); // Ensure scroll view is also tagged

	// Create a layout to hold view selection buttons
	auto viewListLayout = ui::Layout::create();
	viewListLayout->setLayoutType(ui::Layout::Type::VERTICAL);
	viewListLayout->setContentSize(Size(200, mAvailableViews.size() * 30)); // Adjust height based on number of views
	viewListLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
	viewListLayout->setBackGroundColor(Color3B::GRAY);
	viewScrollView->addChild(viewListLayout);
	viewListLayout->setTag(9999); // Ensure layout is also tagged

	float currentY = viewListLayout->getContentSize().height - 30; // Start from top
	for (const auto& viewPair : mAvailableViews)
	{
		auto viewButton = ui::Button::create();
		viewButton->setTitleText(viewPair.first);
		viewButton->setTitleFontSize(18);
		viewButton->setContentSize(Size(180, 25));
		viewButton->setPosition(Vec2(viewListLayout->getContentSize().width / 2, currentY));
		viewButton->addTouchEventListener([&, viewID = viewPair.second](Ref* sender, ui::Widget::TouchEventType type){
			if (type == ui::Widget::TouchEventType::ENDED)
			{
				// Load the selected view
				this->loadViewObjects(viewID); // Use the stored view ID
				this->mCurrentViewID = viewID; // Update the current view ID
			}
		});
		viewListLayout->addChild(viewButton);
		viewButton->setTag(9999); // Ensure button is also tagged
		currentY -= 30;
	}

	// Add Node button
	auto addNodeButton = ui::Button::create();
	addNodeButton->setTitleText("Add Node");
	addNodeButton->setTitleFontSize(20);
	addNodeButton->setPosition(Vec2(origin.x + addNodeButton->getContentSize().width / 2 + 10, visibleSize.height - 30));
	addNodeButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type){
		if (type == ui::Widget::TouchEventType::ENDED)
		{
			addNodeToScene("Node");
		}
	});
	this->addChild(addNodeButton, 9999);

	// Add Sprite button
	auto addSpriteButton = ui::Button::create();
	addSpriteButton->setTitleText("Add Sprite");
	addSpriteButton->setTitleFontSize(20);
	addSpriteButton->setPosition(Vec2(origin.x + addNodeButton->getContentSize().width + addSpriteButton->getContentSize().width / 2 + 20, visibleSize.height - 30));
	addSpriteButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type){
		if (type == ui::Widget::TouchEventType::ENDED)
		{
			addNodeToScene("Sprite", "HelloWorld.png"); // Example texture
		}
	});
	this->addChild(addSpriteButton, 9999);

	// Save Scene button
	auto saveButton = ui::Button::create();
	saveButton->setTitleText("Save Scene");
	saveButton->setTitleFontSize(20);
	saveButton->setPosition(Vec2(visibleSize.width - saveButton->getContentSize().width / 2 - 10, visibleSize.height - 30));
	saveButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type){
		if (type == ui::Widget::TouchEventType::ENDED)
		{
			saveViewObjects(this->mCurrentViewID); // Use the current view ID
		}
	});
	this->addChild(saveButton, 9999);

	// Back to Game button
	auto backButton = ui::Button::create();
	backButton->setTitleText("Back to Game");
	backButton->setTitleFontSize(20);
	backButton->setPosition(Vec2(visibleSize.width - backButton->getContentSize().width / 2 - 10, visibleSize.height - 80));
	backButton->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type){
		if (type == ui::Widget::TouchEventType::ENDED)
		{
			SM->runGameScene(); // We will implement runGameScene in ScenesManager
		}
	});
	this->addChild(backButton, 9999);

	// Basic touch event for selecting nodes
	auto touchListener = EventListenerTouchOneByOne::create();
	touchListener->onTouchBegan = [&](Touch* touch, Event* event) {
		Vec2 touchLocation = touch->getLocation();
		Node* hitNode = nullptr;
		for (Node* child : this->getChildren())
		{
			if (child->getBoundingBox().containsPoint(touchLocation) && child->getTag() != 9999)
			{
				hitNode = child;
				break;
			}
		}
		mSelectedNode = hitNode;
		return true;
	};
	touchListener->onTouchMoved = [&](Touch* touch, Event* event) {
		if (mSelectedNode)
		{
			mSelectedNode->setPosition(touch->getLocation());
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

void EditorScene::addNodeToScene(const std::string& type, const std::string& textureFileName)
{
	sSceneObjectInfo objInfo;
	objInfo.type = type;
	objInfo.name = type + std::to_string(this->getChildrenCount()); // Simple unique name
	objInfo.position = Director::getInstance()->getVisibleSize() / 2;
	objInfo.textureFileName = textureFileName;

	if (type == "Button" && !textureFileName.empty())
	{
		BValue customData;
		customData.clearWithType(BValue::Type::MAP);
		customData.getValueMap()["textureFileName"] = BValue(textureFileName);
		objInfo.customData = customData;
	}

	Node* newNode = NodeHelper::createNodeFromSceneObjectInfo(objInfo);
	if (newNode)
	{
		this->addChild(newNode);
		mSelectedNode = newNode;
	}
}
