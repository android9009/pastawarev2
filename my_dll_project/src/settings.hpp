#pragma once

// Cheat settings consumed by the menu (and, later, by feature code).
// Every entry is an xui::setting so the menu's checkbox/keybind/search
// machinery works out of the box. Plain float/int variables back sliders
// and combos.

#include "pch.hpp"
#include "../external/xdraw/xui/xui.hpp"

namespace settings {

	// ── ragebot ────────────────────────────────────────────────────────────
	namespace ragebot {

		inline xui::setting enabled{ true, {}, "enabled", "ragebot" };
		inline xui::setting silent{ true, {}, "silent", "ragebot" };
		inline xui::setting auto_scope{ false, {}, "auto scope", "ragebot" };
		inline xui::setting auto_stop{ false, {}, "auto stop", "ragebot" };
		inline xui::setting prefer_lethal{ false, {}, "prefer lethal", "ragebot" };
		inline xui::setting zeusbot{ false, {}, "zeusbot", "ragebot" };
		inline xui::setting knifebot{ false, {}, "knifebot", "ragebot" };
		inline xui::setting revolvo{ false, {}, "revolver", "ragebot" };

		inline int hitbox{ 0 };
		inline float fov{ 12.0f };
		inline float min_damage{ 20.0f };
		inline float hit_chance{ 35.0f };

		inline constexpr const char* hitbox_names[ ]{ "head", "neck", "chest", "pelvis", "legs" };

		struct weapon_group
		{
			xui::setting force_shot{ false };
			xui::setting body_aim{ false };
			xui::setting no_spread{ false };
			xui::setting quick_stop{ true };
			float min_damage{ 20.0f };
			float hit_chance{ 35.0f };
		};

		inline constexpr const char* weapon_names[ ]{ "pistols", "smgs", "rifles", "shotguns", "snipers", "lmgs" };
		inline weapon_group groups[ 6 ]{ };

	} // namespace ragebot

	// ── legitbot ───────────────────────────────────────────────────────────
	namespace legitbot {

		inline xui::setting enabled{ true, {}, "enabled", "legitbot" };
		inline xui::setting visible_check{ true, {}, "visible check", "legitbot" };
		inline xui::setting rcs{ true, {}, "recoil control", "legitbot" };
		inline xui::setting backtrack{ false, {}, "backtrack", "legitbot" };
		inline xui::setting smooth_enabled{ true, {}, "smoothing", "legitbot" };

		inline int hitbox{ 0 };
		inline float fov{ 3.0f };
		inline float smooth{ 8.0f };
		inline float rcs_amt{ 50.0f };
		inline float backtrack_ms{ 200.0f };

		inline constexpr const char* hitbox_names[ ]{ "head", "neck", "chest", "pelvis", "legs" };

	} // namespace legitbot

	// ── player visuals ─────────────────────────────────────────────────────
	namespace player {

		struct layer
		{
			xui::setting enabled{ false };
			xdraw::color color{ 173, 192, 255, 255 };
		};

		struct overlay
		{
			xui::setting enabled{ true, {}, "esp overlay", "enemies" };

			struct
			{
				xui::setting enabled{ true, {}, "bounding box", "enemies" };
				int style{ 0 };
				xui::setting fill{ false, {}, "box fill", "enemies" };
				xui::setting outline{ true, {}, "box outline", "enemies" };
				float corner_length{ 8.0f };
			} m_box;

			layer m_name{ };
			layer m_weapon{ };
			layer m_skeleton{ };
			layer m_glow{ };
			layer m_chams{ };
			layer m_oof{ };

			struct
			{
				xui::setting enabled{ true, {}, "health bar", "enemies" };
				int position{ 0 };
				xui::setting gradient{ true, {}, "health gradient", "enemies" };
				xui::setting show_value{ false, {}, "health value", "enemies" };
			} m_health;

			struct
			{
				xui::setting enabled{ false, {}, "ammo bar", "enemies" };
				int position{ 0 };
			} m_ammo;

			struct
			{
				xui::setting enabled{ false, {}, "info flags", "enemies" };
				bool flags[ 8 ]{ true, true, false, false, false, false, false, false };
			} m_flags;

			struct
			{
				xui::setting enabled{ false, {}, "radar", "enemies" };
				float range{ 600.0f };
			} m_radar;
		};

		inline overlay m_overlay[ 2 ]{ }; // 0 = enemies, 1 = allies

		inline constexpr const char* flag_names[ ]{ "money", "armor", "kit", "scoped", "defusing", "flashed", "ping", "distance" };
		inline constexpr const char* box_styles[ ]{ "full", "cornered" };
		inline constexpr const char* bar_positions[ ]{ "left", "top", "bottom" };

	} // namespace player

	// ── world ──────────────────────────────────────────────────────────────
	namespace world {

		// esp
		inline xui::setting smoke_removal{ false, {}, "smoke removal", "world" };
		inline xui::setting flash_removal{ false, {}, "flash removal", "world" };
		inline xui::setting no_scope{ false, {}, "no scope", "world" };
		inline xui::setting bomb_timer{ true, {}, "bomb timer", "world" };
		inline xui::setting molotov_timer{ true, {}, "molotov timer", "world" };
		inline xui::setting dropped_weapons{ true, {}, "dropped weapons", "world" };
		inline xui::setting grenade_projectiles{ true, {}, "grenade projectiles", "world" };
		inline xui::setting grenade_tracers{ true, {}, "grenade tracers", "world" };
		inline xui::setting impacts{ true, {}, "bullet impacts", "world" };
		inline xui::setting spectator_list{ false, {}, "spectator list", "world" };
		inline xui::setting player_count{ false, {}, "player count", "world" };

		// scene
		inline xui::setting night_mode{ false, {}, "night mode", "scene" };
		inline xui::setting no_fog{ false, {}, "no fog", "scene" };
		inline xui::setting no_sky{ false, {}, "no sky", "scene" };
		inline xui::setting bright{ false, {}, "brightness", "scene" };
		inline xui::setting ambient_light{ false, {}, "ambient light", "scene" };
		inline float scene_brightness{ 1.0f };

		// weather
		inline xui::setting rain{ false, {}, "rain", "weather" };
		inline xui::setting snow{ false, {}, "snow", "weather" };
		inline xui::setting stars{ false, {}, "stars", "weather" };
		inline float weather_intensity{ 1.0f };

	} // namespace world

	// ── skins ──────────────────────────────────────────────────────────────
	namespace skins {

		inline xui::setting enabled{ true, {}, "skins enabled", "skins" };
		inline int seed{ 0 };
		inline float wear{ 0.02f };
		inline xui::setting stattrak{ false, {}, "stattrak", "skins" };
		inline xui::setting custom_name{ false, {}, "custom name", "skins" };
		inline std::string name_tag{ "pastaware" };
		inline int paint_kit{ 0 };

		inline xui::setting knife_enabled{ true, {}, "knife enabled", "knives" };
		inline int knife_model{ 0 };
		inline int knife_paint_kit{ 0 };

		inline xui::setting gloves_enabled{ false, {}, "gloves enabled", "gloves" };
		inline int gloves_model{ 0 };
		inline int gloves_paint_kit{ 0 };

		inline xui::setting agents_enabled{ false, {}, "agents enabled", "agents" };
		inline int agent_model{ 0 };

		inline constexpr const char* knife_names[ ]{ "karambit", "m9 bayonet", "bayonet", "butterfly", "stiletto", "talon", "ursus", "navaja" };
		inline constexpr const char* glove_names[ ]{ "sport gloves", "driver gloves", "moto gloves", "specialist gloves", "bloodhound gloves", "hand wraps" };

	} // namespace skins

	// ── misc ───────────────────────────────────────────────────────────────
	namespace misc {

		// main
		inline xui::setting watermark{ true, {}, "watermark", "main" };
		inline xui::setting bhop{ false, {}, "bunny hop", "main" };
		inline xui::setting auto_strafe{ false, {}, "auto strafe", "main" };
		inline xui::setting edge_jump{ false, {}, "edge jump", "main" };
		inline xui::setting edge_bug{ false, {}, "edge bug", "main" };
		inline xui::setting fast_ladder{ false, {}, "fast ladder", "main" };
		inline xui::setting slow_walk{ false, {}, "slow walk", "main" };
		inline xui::setting auto_peek{ false, {}, "auto peek", "main" };
		inline xui::setting radar_hack{ true, {}, "radar hack", "main" };
		inline xui::setting reveal_ranks{ false, {}, "reveal ranks", "main" };

		// removals
		inline xui::setting remove_smoke{ false, {}, "remove smoke", "removals" };
		inline xui::setting remove_flash{ false, {}, "remove flash", "removals" };
		inline xui::setting remove_scope{ false, {}, "remove scope", "removals" };
		inline xui::setting remove_postprocess{ false, {}, "remove postprocess", "removals" };
		inline xui::setting remove_shadows{ false, {}, "remove shadows", "removals" };

		// camera
		inline xui::setting thirdperson{ false, {}, "thirdperson", "camera" };
		inline float fov{ 110.0f };
		inline float viewmodel_fov{ 68.0f };
		inline xui::setting camera_smoothing{ false, {}, "camera smoothing", "camera" };

		// hud
		inline xui::setting hitmarker{ false, {}, "hitmarker", "hud" };
		inline xui::setting damage_indicator{ false, {}, "damage indicator", "hud" };
		inline xui::setting show_fps{ true, {}, "watermark fps", "hud" };
		inline xui::setting show_time{ true, {}, "watermark time", "hud" };
		inline xui::setting show_spectators{ false, {}, "spectators", "hud" };

		inline int menu_key{ VK_INSERT };

	} // namespace misc

	// ── config ─────────────────────────────────────────────────────────────
	namespace config {
		inline xui::setting save_on_exit{ true, {}, "save on exit", "config" };
		inline xui::setting load_on_start{ true, {}, "load on start", "config" };
	}

} // namespace settings
