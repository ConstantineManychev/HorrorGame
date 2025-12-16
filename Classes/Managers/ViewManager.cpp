#include "ViewManager.h"
#include "DataManager.h"
#include "ScenesManager.h"
#include "Basics/BaseLocation.h"
#include "Factories/ViewFactory.h" 

USING_NS_CC;
using namespace GameSpace;

ViewManager::ViewManager()
{
}

ViewManager::~ViewManager()
{
	for (auto& pair : mViews)
	{
		if (pair.second)
		{
			pair.second->release();
		}
	}
	mViews.clear();

	mViewsActions.clear();
}

Node* ViewManager::createViewByID(const std::string& aID)
{
	return VF->createView(aID);;
}

void ViewManager::changeView(const std::string& aViewID)
{
	auto currentScene = SM->getCurrentScene();

	if (currentScene)
	{
		std::vector<std::string> viewsToRemove;
		std::vector<Node*> nodesToRemove;

		for (auto child : currentScene->getChildren())
		{
			if (child->getTag() != 9999 && !dynamic_cast<Camera*>(child))
			{
				if (!child->getName().empty())
				{
					viewsToRemove.push_back(child->getName());
				}
				nodesToRemove.push_back(child);
			}
		}

		for (const auto& id : viewsToRemove)
		{
			removeViewByID(id);
		}

		for (auto node : nodesToRemove)
		{
			node->removeFromParent();
		}

		auto view = getViewByID(aViewID);

		if (view)
		{
			currentScene->addChild(view);

			auto location = dynamic_cast<BaseLocation*>(view);
			if (location)
			{
				location->onOpen();
			}

			currentScene->setCurrentViewID(aViewID);
		}
	}
}

void ViewManager::removeViewByID(const std::string& aID)
{
	auto it = mViews.find(aID);
	if (it != mViews.end())
	{
		Node* node = it->second;
		cleanupNodeActions(node);
		mViews.erase(it);
		if (node)
		{
			node->release();
		}
	}
}

Node* ViewManager::getViewByID(const std::string& aID)
{
	auto it = mViews.find(aID);
	if (it != mViews.end())
	{
		return it->second;
	}

	Node* newNode = createViewByID(aID);
	if (newNode)
	{
		newNode->retain();
		mViews[aID] = newNode;
		return newNode;
	}

	return nullptr;
}

void ViewManager::runActionForNode(Node* aNode, const std::string& aID)
{
	if (aNode)
	{
		auto actionsListIt = mViewsActions.find(aNode);
		if (actionsListIt != mViewsActions.end())
		{
			auto& actionsList = actionsListIt->second;
			auto actionIt = actionsList.find(aID);
			if (actionIt != actionsList.end())
			{
				auto sequence = Sequence::create(actionIt->second);
				aNode->runAction(sequence);
			}
		}
	}
}

void ViewManager::cleanupNodeActions(Node* aNode)
{
	if (aNode)
	{
		mViewsActions.erase(aNode);
	}
}