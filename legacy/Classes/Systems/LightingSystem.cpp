#include "LightingSystem.h"

USING_NS_CC;
_CSTART

LightingSystem::LightingSystem()
	: mRenderTexture(nullptr), mDarknessLayer(nullptr), mAmbientColor(0.05f, 0.05f, 0.1f, 1.0f)
{}

LightingSystem::~LightingSystem() {}

void LightingSystem::initLighting(Size mapSize)
{
	mRenderTexture = RenderTexture::create(mapSize.width, mapSize.height);
	this->addChild(mRenderTexture);

	mDarknessLayer = Sprite::createWithTexture(mRenderTexture->getSprite()->getTexture());
	mDarknessLayer->setTextureRect(Rect(0, 0, mapSize.width, mapSize.height));
	mDarknessLayer->setPosition(mapSize.width / 2, mapSize.height / 2);

	mDarknessLayer->setFlippedY(true);

	BlendFunc blend = { backend::BlendFactor::DST_COLOR, backend::BlendFactor::ZERO };
	mDarknessLayer->setBlendFunc(blend);

}

void LightingSystem::addLight(LightComponent* light) { mLights.push_back(light); }
void LightingSystem::removeLight(LightComponent* light)
{
	auto it = std::find(mLights.begin(), mLights.end(), light);
	if (it != mLights.end()) mLights.erase(it);
}

void LightingSystem::setAmbientColor(const Color3B& color)
{
	mAmbientColor = Color4F(color);
}

void LightingSystem::update(float dt)
{
	if (!mRenderTexture) return;

	mRenderTexture->beginWithClear(mAmbientColor.r, mAmbientColor.g, mAmbientColor.b, mAmbientColor.a);

	for (auto light : mLights)
	{
		auto sprite = light->getLightSprite();
		auto owner = light->getOwner();

		if (sprite && owner)
		{
			sprite->setPosition(owner->getPosition());
			sprite->setRotation(owner->getRotation());
			sprite->setScale(owner->getScale()); 

			sprite->visit();
		}
	}
	mRenderTexture->end();
}

_CEND