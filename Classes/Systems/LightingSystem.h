#ifndef __LIGHTING_SYSTEM_H__
#define __LIGHTING_SYSTEM_H__

#include "cocos2d.h"
#include "Basics/ServiceLocator.h"
#include "Components/LightComponent.h"
#include <vector>

_CSTART

class LightingSystem : public cocos2d::Node
{
	friend class ServiceLocator;
	friend class AppDelegate;

public:

	void initLighting(cocos2d::Size mapSize);

	void update(float dt) override;

	void addLight(LightComponent* light);
	void removeLight(LightComponent* light);

	void setAmbientColor(const cocos2d::Color3B& color);

	cocos2d::Sprite* getDarknessLayer() const { return mDarknessLayer; }

private:
	LightingSystem();
	~LightingSystem();

	std::vector<LightComponent*> mLights;
	cocos2d::RenderTexture* mRenderTexture;
	cocos2d::Sprite* mDarknessLayer;
	cocos2d::Color4F mAmbientColor;
};

#define LIGHT_SYS SL->getService<LightingSystem>()

_CEND

#endif