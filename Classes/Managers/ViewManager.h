#ifndef __VIEW_MANAGER_H__
#define __VIEW_MANAGER_H__

#include <unordered_set>

#include "cocos2d.h"

#include "CommonDefines.h"

#include "Basics/BValue.h"
#include "Basics/BaseAction.h"

_CSTART

class ViewManager
{
	enum class Params
	{
		NONE = 0,
		CHILDREN,
		PARAMS,
		ACTIONS,

		ID,
		RES,
		RES_NORMAL,
		RES_PRESSED,
		RES_DISABLE,

		LAYER,
		OPACITY,

		IS_VISIBLE,

		POS_X,
		POS_Y,
		ANCH_X,
		ANCH_Y,
	};

private:

	ViewManager();

	const std::unordered_set<std::string> cExcludeParams = { "children", "actions", "creation" };
	const std::unordered_map<std::string, Params> cParamTypeMap = {
		{"children", Params::CHILDREN}, {"params", Params::PARAMS}, {"actions", Params::ACTIONS},
		{"id", Params::ID}, {"res", Params::RES},
		{"res_normal", Params::RES_NORMAL},{"res_pressed", Params::RES_PRESSED},{"res_disable", Params::RES_DISABLE},
		{"layer", Params::LAYER}, {"opacity", Params::OPACITY},
		{"pos_x", Params::POS_X}, {"pos_y", Params::POS_Y},{"anch_x", Params::ANCH_X}, {"anch_y", Params::ANCH_Y}
	};

	std::unordered_map <std::string, Node*> mViews;
	std::unordered_map < Node*, std::unordered_map < std::string, Vector< FiniteTimeAction* > > > mViewsActions;

	Node* createNodeFromBValue(const BValue& aBValue, Node* aParentNode = nullptr);
	void fillNodeParamFromBValue(Node* aNode, const std::string& aParamID, const BValue& aBValue);

	FiniteTimeAction* createActionFromBValue(const BValue& aBValue, Node* aNode);

	void changeView(const std::string& aViewID);

	bool isAllConditionsMeetRequirements(const BValueMap& aMap, Node* aNode);

	const BValueMap& getParamMap(const BValueMap& aMap, const std::string& aParam);
	std::string getParamString(const BValueMap& aMap, const std::string& aParam);
	float getParamFloat(const BValueMap& aMap, const std::string& aParam);
	bool getParamBool(const BValueMap& aMap, const std::string& aParam);

public:

	static ViewManager* getInstance();

	Node* createViewByID(const std::string& aID);

	void removeViewByID(const std::string& aID);

	Node* getViewByID(const std::string& aID);

	void runActionForNode(Node* aNode, const std::string& aID);

};

#define VM ViewManager::getInstance()

_CEND

#endif // __VIEW_MANAGER_H__
