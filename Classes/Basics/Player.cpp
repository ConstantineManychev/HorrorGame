#include "Player.h"
#include "Managers/GameDirector.h"

USING_NS_CC;
_CSTART

Player* Player::create()
{
	Player* pRet = new(std::nothrow) Player();
	if (pRet && pRet->init())
	{
		pRet->autorelease();
		return pRet;
	}
	else
	{
		delete pRet;
		pRet = nullptr;
		return nullptr;
	}
}

bool Player::init()
{
	if (!GameEntity::init()) return false;

	if (!this->getComponent("PhysicsBodyComponent"))
	{
		auto bodyComp = PhysicsBodyComponent::create(true);
		this->addComponent(bodyComp);
	}

	mMovement = PhysicsMovementComponent::create();
	this->addComponent(mMovement);

	mInput = InputComponent::create();
	mInput->setActionCallback(CC_CALLBACK_2(Player::onInputAction, this));
	this->addComponent(mInput);

	return true;
}

void Player::onInputAction(GameAction action, bool isPressed)
{
	float speed = 200.0f;
	if (isPressed)
	{
		switch (action)
		{
		case GameAction::MoveLeft: mMovement->addVelocity(Vec2(-speed, 0)); break;
		case GameAction::MoveRight: mMovement->addVelocity(Vec2(speed, 0)); break;
		case GameAction::MoveUp: mMovement->addVelocity(Vec2(0, speed)); break;
		case GameAction::MoveDown: mMovement->addVelocity(Vec2(0, -speed)); break;
		default: break;
		}
	}
	else
	{
		// ??????? ?????????. ??? ????? ???????? ?????????? ????? ???????? ??????
		// (?????????, ????? ??????? ??? ??? ??????)
		mMovement->setVelocity(Vec2::ZERO);
	}
}

_CEND