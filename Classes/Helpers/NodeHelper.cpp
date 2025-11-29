
#include "ui/UIButton.h"

#include "NodeHelper.h"
#include "Basics/BaseLocation.h"
#include "Basics/Enemy.h"
#include "Basics/Player.h"
#include "Basics/BaseLocation.h"
#include "Logics/LocationLogic.h"
#include "Basics/Parallax.h"
#include "Managers/DataManager.h"


USING_NS_CC;
_CSTART

Node* NodeHelper::createNodeForType(const std::string& aType)
{
	Node* result = nullptr;
	if (aType == "Node")
	{
		result = Node::create();
	}
	else if (aType == "Sprite")
	{
		result = Sprite::create();
	}
	else if (aType == "Button")
	{
		ui::Button* btn = ui::Button::create();

		result = btn;
	}
	else if (aType == "Location")
	{
		result = BaseLocation::create();
	}
	else if (aType == "Player")
	{
		result = Player::create();
	}
	else if (aType == "Enemy")
	{
		result = Enemy::create();
	}
	else if (aType == "Parallax")
	{
		result = Parallax::create();
	}

	return result;
}

Node* NodeHelper::createNodeFromSceneObjectInfo(const sSceneObjectInfo& objectInfo)
{
	Node* node = nullptr;

	if (objectInfo.type == "Sprite" && !objectInfo.textureFileName.empty())
	{
		node = Sprite::create(objectInfo.textureFileName);
	}
	else if (objectInfo.type == "Button" && !objectInfo.textureFileName.empty())
	{
		ui::Button* btn = ui::Button::create(objectInfo.textureFileName);
		// You might want to set other button states (pressed, disabled) here as well
		node = btn;
	}
	else
	{
		node = createNodeForType(objectInfo.type);
	}

	if (node)
	{
		node->setName(objectInfo.name);
		node->setPosition(objectInfo.position);
		node->setScaleX(objectInfo.scaleX);
		node->setScaleY(objectInfo.scaleY);
		node->setLocalZOrder(objectInfo.zOrder);

		// Handle custom data if needed
		// For example: if (objectInfo.customData.isMap()) { ... }
	}

	return node;
}

void NodeHelper::stopAllActionsRecursive(Node* aNode)
{
	if (aNode)
	{
		aNode->stopAllActions();

		for (auto& child : aNode->getChildren())
		{
			stopAllActionsRecursive(child);
		}
	}
}

_CEND