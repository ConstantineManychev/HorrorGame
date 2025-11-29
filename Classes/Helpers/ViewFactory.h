#ifndef __VIEW_FACTORY_H__
#define __VIEW_FACTORY_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include <unordered_set>
#include <unordered_map>

namespace GameSpace {

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
			SCALE_X,
			SCALE_Y,
			ROTATION,
			ANCH_X,
			ANCH_Y,

			COLOR
		};

		// Заменили BValue на cocos2d::Value
		static cocos2d::Node* createNodeFromValue(const cocos2d::Value& aValue, cocos2d::Node* aParentNode = nullptr);

		static void parseActions(const cocos2d::Value& aValue, cocos2d::Node* aNode, std::unordered_map<cocos2d::Node*, std::unordered_map<std::string, cocos2d::Vector<cocos2d::FiniteTimeAction*>>>& outActionsMap);

	private:
		static void fillNodeParamFromValue(cocos2d::Node* aNode, const std::string& aParamID, const cocos2d::Value& aValue, std::unordered_map<cocos2d::Node*, std::unordered_map<std::string, cocos2d::Vector<cocos2d::FiniteTimeAction*>>>& outActionsMap, cocos2d::Node* aParentNode);
		static cocos2d::FiniteTimeAction* createActionFromValue(const cocos2d::Value& aValue, cocos2d::Node* aNode);

		static const std::unordered_set<std::string> cExcludeParams;
		static const std::unordered_map<std::string, Params> cParamTypeMap;

		static bool isAllConditionsMeetRequirements(const cocos2d::ValueMap& aMap, cocos2d::Node* aNode);
	};

} // namespace GameSpace

#endif // __VIEW_FACTORY_H__