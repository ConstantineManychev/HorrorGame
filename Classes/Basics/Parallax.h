#ifndef __PARALLAX_H__
#define __PARALLAX_H__

#include "cocos2d.h"
#include "CommonDefines.h"

_CSTART

class Parallax : public cocos2d::Node
{
public:
	static Parallax* create();

	virtual bool init() override;
	virtual void update(float delta) override;

	void addChildWithRatio(cocos2d::Node* aNode, const cocos2d::Vec2& aRatio, const cocos2d::Vec2& aOffset);

private:
	struct ParallaxLayerInfo
	{
		cocos2d::Node* node;
		cocos2d::Vec2 ratio;
		cocos2d::Vec2 offset;
	};

	std::vector<ParallaxLayerInfo> mLayers;
};

_CEND

#endif // __PARALLAX_H__