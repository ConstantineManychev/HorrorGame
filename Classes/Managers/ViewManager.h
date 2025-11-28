#ifndef __VIEW_MANAGER_H__
#define __VIEW_MANAGER_H__

#include <unordered_set>
#include <unordered_map>
#include "cocos2d.h"
#include "CommonDefines.h"
#include "Basics/BValue.h"
#include "Basics/BaseAction.h"

_CSTART

class ViewManager
{
public:
	static ViewManager* getInstance();

	// Lifecycle
	Node* createViewByID(const std::string& aID);
	void removeViewByID(const std::string& aID);
	Node* getViewByID(const std::string& aID);
	void changeView(const std::string& aViewID);

	// Actions
	void runActionForNode(Node* aNode, const std::string& aID);
	
	// Accessor for Factory (Friendship preferred but public for simplicity in refactoring)
	std::unordered_map<Node*, std::unordered_map<std::string, Vector<FiniteTimeAction*>>>& getActionsMap() { return mViewsActions; }

private:
	ViewManager();
	~ViewManager(); // Added Destructor for cleanup

	std::unordered_map<std::string, Node*> mViews;
	std::unordered_map<Node*, std::unordered_map<std::string, Vector<FiniteTimeAction*>>> mViewsActions;

	void cleanupNodeActions(Node* aNode);
};

#define VM ViewManager::getInstance()

_CEND

#endif // __VIEW_MANAGER_H__
