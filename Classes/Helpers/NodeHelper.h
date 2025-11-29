#ifndef __NODE_HELPER_H__
#define __NODE_HELPER_H__

#include "cocos2d.h"

#include "CommonDefines.h"
#include "Types/BasicDataTypes.h"

_CSTART

class NodeHelper
{
public:

	static cocos2d::Node* createNodeForType(const std::string& aType);
	static cocos2d::Node* createNodeFromSceneObjectInfo(const sSceneObjectInfo& objectInfo);

	static void stopAllActionsRecursive(cocos2d::Node* aNode);

};

_CEND

#endif // __NODE_HELPER_H__
