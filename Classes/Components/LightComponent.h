#ifndef __LIGHT_COMPONENT_H__
#define __LIGHT_COMPONENT_H__

#include "GameComponent.h"

_CSTART

class LightComponent : public GameComponent
{
public:
	static LightComponent* create(const std::string& texturePath);

	virtual bool init() override;
	virtual void onEnter() override;
	virtual void onExit() override;

	cocos2d::Sprite* getLightSprite() const { return mLightSprite; }

	void setColor(const cocos2d::Color3B& color);
	void setOpacity(GLubyte opacity);
	void setScale(float scale);

	void enableFlicker(bool enable);

private:
	std::string mTexturePath;
	cocos2d::Sprite* mLightSprite;
	bool mIsFlickerEnabled;

	void updateFlicker(float dt);
};

_CEND

#endif