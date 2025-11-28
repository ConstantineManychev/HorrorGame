#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "BaseBeing.h"
#include "Components/InputComponent.h"
#include "Components/MovementComponent.h"

_CSTART

class Player : public BaseBeing
{
public:
	static Player* create();
	
	virtual bool init() override;

private:
	InputComponent* mInput;
	MovementComponent* mMovement;
	
	void onInputAction(GameAction action, bool isPressed);
};

_CEND

#endif // __PLAYER_H__
