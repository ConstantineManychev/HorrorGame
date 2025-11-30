#include "NodeHelper.h"
#include "ui/UIButton.h"
#include "Factories/EntityFactory.h" 
#include "Managers/DataManager.h"

USING_NS_CC;
_CSTART

Node* NodeHelper::createNodeForType(const std::string& aType)
{
	return EF->createEntity(aType);
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
		node = btn;
	}
	else
	{
		// Use factory for everything else
		node = createNodeForType(objectInfo.type);
	}

	if (node)
	{
		node->setName(objectInfo.name);
		node->setPosition(objectInfo.position);
		node->setScaleX(objectInfo.scaleX);
		node->setScaleY(objectInfo.scaleY);
		node->setLocalZOrder(objectInfo.zOrder);
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