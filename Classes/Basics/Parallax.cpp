#include "Parallax.h"

USING_NS_CC;
_CSTART

Parallax* Parallax::create()
{
	Parallax* ret = new (std::nothrow) Parallax();
	if (ret && ret->init())
	{
		ret->autorelease();
		return ret;
	}
	delete ret;
	return nullptr;
}

bool Parallax::init()
{
	if (!Node::init()) return false;

	this->scheduleUpdate();

	return true;
}

void Parallax::addChildWithRatio(Node* aNode, const Vec2& aRatio, const Vec2& aOffset)
{
	if (!aNode) return;

	this->addChild(aNode);

	ParallaxLayerInfo info;
	info.node = aNode;
	info.ratio = aRatio;
	info.offset = aOffset;

	aNode->setPosition(aOffset);

	mLayers.push_back(info);
}

void Parallax::update(float delta)
{
	auto scene = Director::getInstance()->getRunningScene();
	if (!scene) return;

	auto camera = scene->getDefaultCamera();
	if (!camera) return;

	Vec2 camPos = camera->getPosition();

	for (const auto& layer : mLayers)
	{
		if (layer.node)
		{

			float x = camPos.x * (1.0f - layer.ratio.x) + layer.offset.x;
			float y = camPos.y * (1.0f - layer.ratio.y) + layer.offset.y;

			layer.node->setPosition(Vec2(x, y));
		}
	}
}

_CEND