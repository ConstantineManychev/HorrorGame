#ifndef __PARALLAX_H__
#define __PARALLAX_H__

#include "BaseObject.h"

#include "CommonDefines.h"

_CSTART

class Parallax : public BaseObject
{
	typedef BaseObject Parent;

protected:

	void update(float delta) override;
public:

};

_CEND

#endif // __ENEMY_H__
