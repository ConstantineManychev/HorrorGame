#ifndef __VIEW_FACTORY_H__
#define __VIEW_FACTORY_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Basics/BValue.h"
#include <unordered_set>
#include <unordered_map>

_CSTART

class ViewFactory
{
public:
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

	static Node* createNodeFromBValue(const BValue& aBValue, Node* aParentNode = nullptr);
	
	// Парсинг действий вынесен в публичный метод, чтобы ViewManager мог использовать его отдельно
	static void parseActions(const BValue& aBValue, Node* aNode, std::unordered_map<Node*, std::unordered_map<std::string, Vector<FiniteTimeAction*>>>& outActionsMap);

private:
	// Added aParentNode parameter to correctly calculate relative positions
	static void fillNodeParamFromBValue(Node* aNode, const std::string& aParamID, const BValue& aBValue, std::unordered_map<Node*, std::unordered_map<std::string, Vector<FiniteTimeAction*>>>& outActionsMap, Node* aParentNode);
	static FiniteTimeAction* createActionFromBValue(const BValue& aBValue, Node* aNode);

	static const std::unordered_set<std::string> cExcludeParams;
	static const std::unordered_map<std::string, Params> cParamTypeMap;

	// Хелперы для парсинга
	static const BValueMap& getParamMap(const BValueMap& aMap, const std::string& aParam);
	static std::string getParamString(const BValueMap& aMap, const std::string& aParam);
	static float getParamFloat(const BValueMap& aMap, const std::string& aParam);
	static bool getParamBool(const BValueMap& aMap, const std::string& aParam);
	static bool isAllConditionsMeetRequirements(const BValueMap& aMap, Node* aNode);
};

_CEND

#endif // __VIEW_FACTORY_H__
