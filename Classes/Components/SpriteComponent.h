#ifndef __SPRITE_COMPONENT_H__
#define __SPRITE_COMPONENT_H__

#include "GameComponent.h"

_CSTART

class SpriteComponent : public GameComponent
{
public:
	static SpriteComponent* create();

	virtual bool init() override;
	virtual void onEnter() override;

	void configure(const cocos2d::ValueMap& config);

	void setTexture(const std::string& filename);
	void setSpriteFrame(const std::string& frameName);

	cocos2d::Sprite* getSprite() const { return mSprite; }

private:
	cocos2d::Sprite* mSprite;
	std::string mPendingTexture;
	std::string mPendingFrame;
};

_CEND

#endif