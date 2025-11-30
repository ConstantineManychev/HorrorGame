#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>
#include "CommonDefines.h"

_CSTART

namespace Constants {

	namespace Configs {
		static const std::string MAIN_CONFIG = "configs/main_config.json";
		static const std::string VIEWS_LIST = "configs/views/views_list.json";
		static const std::string NEW_GAME = "configs/new_game_config.json";
	}

	namespace Scenes {
		static const std::string MAIN = "main";
		static const std::string EDITOR = "editor";
		static const std::string TITLE = "title";
	}

	namespace Contexts {
		static const std::string GAME = "Game";
		static const std::string EDITOR = "Editor";
	}

	namespace Actions {
		static const std::string BTN_CLICK_DOWN = "onBtnClickDown";
		static const std::string BTN_CLICK_UP = "onBtnClickUp";
		static const std::string BTN_CLICK_MOVE = "onBtnClickMove";
		static const std::string BTN_CLICK_CANCEL = "onBtnClickCancel";
		static const std::string ON_CREATE = "onCreate";
	}

	namespace Types {
		static const std::string NODE = "Node";
		static const std::string SPRITE = "Sprite";
		static const std::string BUTTON = "Button";
		static const std::string LOCATION = "Location";
		static const std::string PLAYER = "Player";
		static const std::string ENEMY = "Enemy";
		static const std::string PARALLAX = "Parallax";
	}

	namespace Keys {
		static const std::string TEXTURE_FILE = "textureFileName";
		static const std::string PARTICLE_PATH = "tale_paticle_path";

		static const std::string LEFT = "left";
		static const std::string RIGHT = "right";
		static const std::string UP = "up";
		static const std::string DOWN = "down";
	}
}

_CEND

#endif // __CONSTANTS_H__