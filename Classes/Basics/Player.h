#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "BaseBeing.h"

#include "CommonDefines.h"

_CSTART

class Player : public BaseBeing
{
public:

	static Player* create();

	
};

_CEND

#endif // __PLAYER_H__
