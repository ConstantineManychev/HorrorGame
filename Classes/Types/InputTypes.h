#ifndef __INPUT_TYPES_H__
#define __INPUT_TYPES_H__

#include "CommonDefines.h"

_CSTART

enum class GameAction
{
    None = 0,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Jump,
    Fire,
    Use,
    Pause,
    // UI Actions
    UI_Select,
    UI_Back,
    // ... add more
};

enum class InputDeviceType
{
    KeyboardMouse,
    Gamepad,
    TouchScreen
};

_CEND

#endif // __INPUT_TYPES_H__

