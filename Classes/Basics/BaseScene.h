#ifndef __BASE_SCENE_H__
#define __BASE_SCENE_H__

#include "cocos2d.h"
#include "CommonDefines.h"

_CSTART

class BaseScene : public Scene
{
	typedef Scene Parent;

public:

    virtual bool init();
    
    void menuCloseCallback(cocos2d::Ref* pSender);

	virtual void useDefaultView() = 0;

	void setCurrentViewID(const std::string& aCurrentViewID) { mCurrentViewID = aCurrentViewID; }
	const std::string& getCurrentViewID() const { return mCurrentViewID; }

	void setSceneFilePath(const std::string& filePath) { mSceneFilePath = filePath; }
	const std::string& getSceneFilePath() const { return mSceneFilePath; }

private:

	std::string mCurrentViewID;
	std::string mSceneFilePath;
};

_CEND

#endif // __BASE_SCENE_H__
