#include "PhysicsBodyComponent.h"

USING_NS_CC;
_CSTART

PhysicsBodyComponent* PhysicsBodyComponent::create(bool isDynamic)
{
	auto pRet = new(std::nothrow) PhysicsBodyComponent();
	if (pRet && pRet->init())
	{
		pRet->setDynamic(isDynamic);
		pRet->autorelease();
		return pRet;
	}
	delete pRet;
	return nullptr;
}

bool PhysicsBodyComponent::init()
{
	if (!GameComponent::init()) return false;
	setName("PhysicsBodyComponent");

	mIsDynamic = true;
	mCategoryBitmask = 0x01;
	mCollisionBitmask = 0x01;
	mContactTestBitmask = 0x00;

	return true;
}

void PhysicsBodyComponent::onEnter()
{
	GameComponent::onEnter();

	auto owner = getOwner();
	if (!owner) return;

	if (owner->getPhysicsBody()) return;

	auto contentSize = owner->getContentSize();
	contentSize.width *= 0.8f;
	contentSize.height *= 0.8f;

	auto body = PhysicsBody::createBox(contentSize, PhysicsMaterial(0.1f, 0.0f, 0.0f));

	if (body)
	{
		body->setDynamic(mIsDynamic);
		body->setRotationEnable(false);
		body->setCategoryBitmask(mCategoryBitmask);
		body->setCollisionBitmask(mCollisionBitmask);
		body->setContactTestBitmask(mContactTestBitmask);

		owner->setPhysicsBody(body);
	}
}

_CEND