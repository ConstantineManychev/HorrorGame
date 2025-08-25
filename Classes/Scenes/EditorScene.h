#ifndef __EDITOR_SCENE_H__
#define __EDITOR_SCENE_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Types/BasicDataTypes.h"

_CSTART

class EditorScene : public cocos2d::Scene
{
public:
	static cocos2d::Scene* createScene(const std::string& viewID);

	virtual bool init() override;

private:
	std::string mCurrentViewID; // ID of the view being edited
	std::vector<std::pair<std::string, std::string>> mAvailableViews; // List of available views (name, view ID)

	void loadViewObjects(const std::string& viewID);
	void saveViewObjects(const std::string& viewID);

	// Editor UI elements
	void setupEditorUI();
	void addNodeToScene(const std::string& type, const std::string& texture = "");

	cocos2d::Node* mSelectedNode; // Currently selected node for editing
};

_CEND

#endif // __EDITOR_SCENE_H__
