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
		static const std::string GAME = "game";
		static const std::string EDITOR = "editor";
		static const std::string TITLE = "title";
	}

	namespace Views {
		static const std::string TITLE = "title_view";
		static const std::string MAIN_MENU = "main_menu_view";
		static const std::string PAUSE_MENU = "pause_menu_view";
		static const std::string HUD = "game_hud_view";
		static const std::string STAGE_A = "stage_A";
	}

	namespace Contexts {
		static const std::string GAME = "Game";
		static const std::string EDITOR = "Editor";
		static const std::string UI = "UI";
	}

	namespace Sounds {
		static const std::string MENU_THEME = "res/sounds/music/menu_theme.mp3";
		static const std::string GAME_THEME = "res/sounds/music/game_theme.mp3";
		static const std::string PAUSE_THEME = "res/sounds/music/pause_theme.mp3";
		static const std::string BUTTON_CLICK = "res/sounds/sfx/click.mp3";
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