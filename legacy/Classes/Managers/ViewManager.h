#ifndef __VIEW_MANAGER_H__
#define __VIEW_MANAGER_H__

#include <unordered_set>
#include <unordered_map>
#include "cocos2d.h"
#include "CommonDefines.h"
#include "Basics/ServiceLocator.h"

namespace GameSpace {

	class ViewManager
	{
		friend class ServiceLocator;
		friend class AppDelegate;

	public:

		cocos2d::Node* getCurrentView() const { return mCurrentView; }

		// Lifecycle
		cocos2d::Node* createViewByID(const std::string& aID);
		void removeViewByID(const std::string& aID);
		cocos2d::Node* getViewByID(const std::string& aID);
		void changeView(const std::string& aViewID);

		// Actions
		void runActionForNode(cocos2d::Node* aNode, const std::string& aID);

		std::unordered_map<cocos2d::Node*, std::unordered_map<std::string, cocos2d::Vector<cocos2d::FiniteTimeAction*>>>& getActionsMap() { return mViewsActions; }

	private:
		ViewManager();
		~ViewManager();

		std::string mCurrentViewID;
		cocos2d::Node* mCurrentView;

		std::unordered_map<std::string, cocos2d::Node*> mViews;
		std::unordered_map<cocos2d::Node*, std::unordered_map<std::string, cocos2d::Vector<cocos2d::FiniteTimeAction*>>> mViewsActions;

		void cleanupNodeActions(cocos2d::Node* aNode);
	};

#define VM SL->getService<ViewManager>()

} // namespace GameSpace

#endif // __VIEW_MANAGER_H__