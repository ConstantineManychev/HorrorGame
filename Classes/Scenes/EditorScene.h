#ifndef __EDITOR_SCENE_H__
#define __EDITOR_SCENE_H__

#include "ui/UIButton.h"
#include "ui/UILayout.h"
#include "ui/UITextField.h"

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
	
	virtual void update(float delta) override;

	CREATE_FUNC(EditorScene);
private:
	// --- Constants & Configuration ---
	struct UIConfig
	{
		static const int TAG_UI_LAYER = 9999;
		static const int TAG_SELECTION_BOX = 9998;

		static constexpr float PANEL_WIDTH = 200.0f;
		static constexpr float PANEL_ITEM_HEIGHT = 35.0f;
		static constexpr float TOOLBAR_HEIGHT = 50.0f;
		static constexpr float PROPERTIES_PANEL_HEIGHT = 300.0f;
		static constexpr float MARGIN = 10.0f;
		static constexpr float BUTTON_HEIGHT = 30.0f;
		static constexpr float BUTTON_WIDTH = 180.0f;
		
		static constexpr int FONT_SIZE_NORMAL = 14;
		static constexpr int FONT_SIZE_BUTTON = 16;
		
		static const cocos2d::Color3B COLOR_PANEL_BG; // Defined in cpp
		static const cocos2d::Color3B COLOR_TOOLBAR_BG;
		static const cocos2d::Color4F COLOR_SELECTION;
	};

	std::string mCurrentViewID;
	std::vector<std::pair<std::string, std::string>> mAvailableViews;
	cocos2d::Node* mSelectedFolder;
	cocos2d::ui::Layout* mNodesListLayout;
	cocos2d::ui::Layout* mPropertiesLayout;

	void loadViewObjects(const std::string& viewID);
	void saveViewObjects(const std::string& viewID);

	// Editor UI elements
	void setupEditorUI();
	void setupPropertiesPanel();
	void updatePropertiesPanel();
	
	void addNodeToScene(const std::string& type, const std::string& texture = "");

	void updateCurrentNodeList(cocos2d::Node* aNode);
	cocos2d::ui::Button* createButtonForChangeFolder(const std::string& aName, cocos2d::Node* aNode, float aCurrentY);

	cocos2d::Node* mSelectedNode;
	cocos2d::DrawNode* mSelectionDrawNode;
    
	cocos2d::Vec2 mTouchOffset;

};

_CEND

#endif // __EDITOR_SCENE_H__
