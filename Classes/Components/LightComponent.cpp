#include "LightComponent.h"
#include "Managers/GameDirector.h" 
#include "Systems/LightingSystem.h"

USING_NS_CC;
_CSTART

LightComponent* LightComponent::create(const std::string& texturePath)
{
	auto pRet = new(std::nothrow) LightComponent();
	if (pRet)
	{
		pRet->mTexturePath = texturePath;
		if (pRet->init())
		{
			pRet->autorelease();
			return pRet;
		}
	}
	delete pRet;
	return nullptr;
}

bool LightComponent::init()
{
	if (!GameComponent::init()) return false;
	setName("LightComponent");
	mIsFlickerEnabled = false;

	mLightSprite = Sprite::create(mTexturePath);
	if (mLightSprite)
	{
		mLightSprite->retain(); 

		BlendFunc blend = { backend::BlendFactor::SRC_ALPHA, backend::BlendFactor::ONE };
		mLightSprite->setBlendFunc(blend);
	}

	return true;
}

void LightComponent::onEnter()
{
	GameComponent::onEnter();
	if (mIsFlickerEnabled)
	{
		Director::getInstance()->getScheduler()->schedule(
			CC_SCHEDULE_SELECTOR(LightComponent::updateFlicker),
			this,
			0.1f,
			false
		);
	}
}

void LightComponent::onExit()
{
	if (mIsFlickerEnabled)
	{
		Director::getInstance()->getScheduler()->unschedule(
			CC_SCHEDULE_SELECTOR(LightComponent::updateFlicker),
			this
		);
	}

	CC_SAFE_RELEASE(mLightSprite);
	GameComponent::onExit();
}

void LightComponent::setColor(const Color3B& color) { if (mLightSprite) mLightSprite->setColor(color); }
void LightComponent::setOpacity(GLubyte opacity) { if (mLightSprite) mLightSprite->setOpacity(opacity); }
void LightComponent::setScale(float scale) { if (mLightSprite) mLightSprite->setScale(scale); }

void LightComponent::enableFlicker(bool enable) { mIsFlickerEnabled = enable; }

void LightComponent::updateFlicker(float dt)
{
	if (!mLightSprite) return;

	int flicker = cocos2d::random(200, 255);
	mLightSprite->setOpacity(flicker);
}

_CEND