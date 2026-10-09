#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "GameEntity.h"
#include "Components/InputComponent.h"
#include "Components/PhysicsBodyComponent.h"
#include "Components/PhysicsMovementComponent.h"
#include "Components/SpriteComponent.h"

_CSTART

class Player : public GameEntity
{
public:
	static Player* create();
	
	virtual bool init() override;

private:
	InputComponent* mInput;
	PhysicsMovementComponent* mMovement;
	
	void onInputAction(GameAction action, bool isPressed);
};

_CEND

#endif // __PLAYER_H__
