#ifndef __EDITOR_SCENE_H__
#define __EDITOR_SCENE_H__

#include "ui/UIButton.h"
#include "ui/UILayout.h"

#include "Basics/BaseScene.h"
#include "CommonDefines.h"
#include "Types/BasicDataTypes.h"

_CSTART

class EditorScene : public BaseScene
{
public:
	static BaseScene* createScene(const std::string& viewID);

	virtual bool init() override;
	virtual void useDefaultView() override;

private:
	std::string mCurrentViewID;
	std::vector<std::pair<std::string, std::string>> mAvailableViews;
	Node* mSelectedFolder;
	ui::Layout* mNodesListLayout;

	void loadViewObjects(const std::string& viewID);
	void saveViewObjects(const std::string& viewID);

	// Editor UI elements
	void setupEditorUI();
	void addNodeToScene(const std::string& type, const std::string& texture = "");

	void updateCurrentNodeList(Node* aNode);
	ui::Button* createButtonForChangeFolder(const std::string& aName, Node* aNode, float aCurrentY);

	cocos2d::Node* mSelectedNode;

	CREATE_FUNC(EditorScene);
};

_CEND

#endif // __EDITOR_SCENE_H__
