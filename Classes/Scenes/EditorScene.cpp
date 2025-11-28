#include "EditorScene.h"
#include "Managers/DataManager.h"
#include "Helpers/NodeHelper.h"

#include "ui/UIScrollView.h"
#include "Managers/ScenesManager.h"
#include "Managers/ViewManager.h"

_USEC

BaseScene* EditorScene::createScene(const std::string& viewID)
{
	EditorScene* scene = static_cast<EditorScene*>(EditorScene::create());
    if (scene)
    {
		scene->setCurrentViewID(viewID);
		scene->setupEditorUI(); 
		//scene->loadViewObjects(scene->mCurrentViewID.empty() ? "main" : scene->mCurrentViewID);
		
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

    return true;
}

void EditorScene::useDefaultView()
{
	VM->changeView("title");
}

void EditorScene::loadViewObjects(const std::string& viewID)
{
	CCLOG("EditorScene: Loading view from: %s", viewID.c_str());

	VM->changeView(viewID);
}

void EditorScene::saveViewObjects(const std::string& viewID)
{
	CCLOG("EditorScene: Saving view to: %s", viewID.c_str());

	std::vector<sSceneObjectInfo> sceneObjects;

	for (Node* child : this->getChildren())
	{
	}

	// Save to the original view config file, not a separate scene file
	DM->saveView(viewID, sceneObjects); // This will require a new DataManager::saveView method
}

void EditorScene::setupEditorUI()
{
	mAvailableViews.clear();

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	std::vector<std::string> viewIDs = DM->getViewsIDs();

	for (const auto& viewID : viewIDs)
	{
		mAvailableViews.push_back({viewID, viewID}); // Store view ID as both name and ID
	}

	auto viewScrollView = ui::ScrollView::create();
	viewScrollView->setContentSize(Size(200, visibleSize.height - 100));
	viewScrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
	viewScrollView->setBounceEnabled(true);
	viewScrollView->setAnchorPoint(Vec2(0, 1));
	viewScrollView->setPosition(Vec2(10, visibleSize.height - 10));
	this->addChild(viewScrollView, 9999);
	viewScrollView->setTag(9999);

	auto viewListLayout = ui::Layout::create();
	viewListLayout->setLayoutType(ui::Layout::Type::VERTICAL);
	viewListLayout->setContentSize(Size(200, mAvailableViews.size() * 30));
	viewListLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
	viewListLayout->setBackGroundColor(Color3B::GRAY);
	viewScrollView->addChild(viewListLayout);
	viewListLayout->setTag(9999);

	float currentY = viewListLayout->getContentSize().height - 30;
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

	auto nodesScrollView = ui::ScrollView::create();
	nodesScrollView->setContentSize(Size(200, visibleSize.height - 100));
	nodesScrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
	nodesScrollView->setBounceEnabled(true);
	nodesScrollView->setAnchorPoint(Vec2(0, 1));
	nodesScrollView->setPosition(Vec2(10, visibleSize.height - 10));
	this->addChild(viewScrollView, 9999);
	nodesScrollView->setTag(9999);

	auto mNodesListLayout = ui::Layout::create();
	mNodesListLayout->setLayoutType(ui::Layout::Type::VERTICAL);
	mNodesListLayout->setContentSize(Size(200, 10 * 30));
	mNodesListLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
	mNodesListLayout->setBackGroundColor(Color3B::GRAY);
	mNodesListLayout->setName("node_list");
	nodesScrollView->addChild(mNodesListLayout);
	mNodesListLayout->setTag(9999);

	updateCurrentNodeList(this);
	

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
	objInfo.name = type + std::to_string(this->getChildrenCount());
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

void EditorScene::updateCurrentNodeList(Node* aNode)
{
	if (mNodesListLayout && aNode)
	{
		mNodesListLayout->removeAllChildrenWithCleanup(true);
		auto currentY = mNodesListLayout->getContentSize().height - 30;
		if (aNode != this && mSelectedFolder)
		{
			auto viewButton = createButtonForChangeFolder("...", mSelectedFolder, currentY);

			currentY -= 30;
		}
		mSelectedFolder = aNode;

		auto viewButton = createButtonForChangeFolder(mSelectedFolder->getName(), mSelectedFolder, currentY);
		currentY -= 30;

		for (Node* child : mSelectedFolder->getChildren())
		{
			auto viewButton = createButtonForChangeFolder(child->getName(), mSelectedFolder, currentY);
			
			currentY -= 30;
		}
	}
}

ui::Button* EditorScene::createButtonForChangeFolder(const std::string& aName, Node* aNode, float aCurrentY)
{
	auto result = ui::Button::create();
	if (aNode && mNodesListLayout)
	{
		const std::string& name = aNode->getName();
		result->setTitleText(name);
		result->setTitleFontSize(18);
		result->setContentSize(Size(180, 25));
		result->setPosition(Vec2(mNodesListLayout->getContentSize().width / 2, aCurrentY));
		result->addTouchEventListener([&, name](Ref* sender, ui::Widget::TouchEventType type)
		{
			if (type == ui::Widget::TouchEventType::ENDED)
			{
				updateCurrentNodeList(aNode);
			}
		});

		mNodesListLayout->addChild(result);
		result->setTag(9999);
	}

	return result;
}
