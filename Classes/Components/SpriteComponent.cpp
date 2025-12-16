#include "SpriteComponent.h"

USING_NS_CC;
_CSTART

SpriteComponent* SpriteComponent::create()
{
	auto pRet = new(std::nothrow) SpriteComponent();
	if (pRet && pRet->init())
	{
		pRet->autorelease();
		return pRet;
	}
	delete pRet;
	return nullptr;
}

bool SpriteComponent::init()
{
	if (!GameComponent::init()) return false;
	setName("SpriteComponent");
	mSprite = nullptr;
	return true;
}

void SpriteComponent::configure(const ValueMap& config)
{
	if (config.count("sprite_frame"))
	{
		setSpriteFrame(config.at("sprite_frame").asString());
	}
	else if (config.count("res"))
	{
		setTexture(config.at("res").asString());
	}
}

void SpriteComponent::setTexture(const std::string& filename)
{
	mPendingTexture = filename;
	mPendingFrame.clear();
	if (mSprite)
	{
		mSprite->setTexture(filename);
		if (mSprite->getTexture()) {
			mSprite->getTexture()->setAliasTexParameters();
		}
	}
}

void SpriteComponent::setSpriteFrame(const std::string& frameName)
{
	mPendingFrame = frameName;
	mPendingTexture.clear();
	if (mSprite)
	{
		mSprite->setSpriteFrame(frameName);
		if (mSprite->getTexture()) {
			mSprite->getTexture()->setAliasTexParameters();
		}
	}
}

void SpriteComponent::onEnter()
{
	GameComponent::onEnter();

	auto owner = getOwner();
	if (!owner) return;

	if (!mSprite)
	{
		mSprite = Sprite::create();

		if (!mPendingFrame.empty()) mSprite->setSpriteFrame(mPendingFrame);
		else if (!mPendingTexture.empty()) mSprite->setTexture(mPendingTexture);

		if (mSprite->getTexture()) {
			mSprite->getTexture()->setAliasTexParameters();
		}

		owner->addChild(mSprite);
	}
}

_CEND