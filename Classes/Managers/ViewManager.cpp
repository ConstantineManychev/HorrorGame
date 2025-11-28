#include "ViewManager.h"
#include "DataManager.h"
#include "ScenesManager.h"
#include "Basics/BaseLocation.h"
#include "Helpers/ViewFactory.h" 

_USEC

ViewManager::ViewManager()
{
}

ViewManager::~ViewManager()
{
	// Cleanup all views
	for (auto& pair : mViews)
	{
		if (pair.second)
		{
			pair.second->release();
		}
	}
	mViews.clear();

	// Cleanup actions
	mViewsActions.clear(); // Vector handles release automatically? 
	// Cocos2d Vector retains elements. When Vector is destroyed or cleared, it releases elements.
	// So simply clearing the map is safer than manual release if Vector is used.
}

ViewManager* ViewManager::getInstance()
{
	static ViewManager instance;
	return &instance;
}

Node* ViewManager::createViewByID(const std::string& aID)
{
	Node* result = nullptr;
	const BValue& viewInfo = DM->getViewInfoByID(aID);

	// Delegate creation to Factory
	result = ViewFactory::createNodeFromBValue(viewInfo);

	if (result)
	{
		result->setScale(DM->getScaleY());
	}

	return result;
}

void ViewManager::changeView(const std::string& aViewID)
{
	auto currentScene = SM->getCurrentScene();

	if (currentScene)
	{
		// Identify views to remove (avoid modifying collection while iterating)
		std::vector<std::string> viewsToRemove;
		for (auto child : currentScene->getChildren())
		{
			if (child->getTag() != 9999 && !dynamic_cast<Camera*>(child)) 
			{
				if (!child->getName().empty())
				{
					viewsToRemove.push_back(child->getName());
				}
			}
		}

		// Remove from Cache and cleanup actions
		for (const auto& id : viewsToRemove)
		{
			removeViewByID(id);
		}
		
		// Remove from Scene
		currentScene->removeAllChildren(); 

		// Add new View
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
		
		// Remove from actions map first
		cleanupNodeActions(node);
		
		// Remove from view cache
		mViews.erase(it);
		
		// Release ownership
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
		newNode->retain(); // Take ownership
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
				// Sequence::create takes variable arguments, ending with nullptr, or a Vector<FiniteTimeAction*>&
				// The Vector version is more efficient and safer here.
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
