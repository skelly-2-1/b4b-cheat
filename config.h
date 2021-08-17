/*
@file

	config.h

@purpose

	Configuration variables
*/

#pragma once

#include "../lib/external_cheat_interfaces/cfg.h"
#include "../lib/strenc/strenc.h"

#pragma push_macro("min")
#pragma push_macro("max")
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace b4b
{

	// note: default values from here will be applied if there's no config setting
	struct config_t final
	{

		// player boneesp
		bool player_bone_esp_active = true;

		color_t player_bone_esp_color = color_t(0, 255, 255);

		uint32_t player_bone_esp_thickness = 1u;

		float player_bone_esp_max_distance = 0.f;

		// zombie boneesp
		bool zombie_bone_esp_active = true;

		color_t zombie_bone_esp_color = color_t(255, 0, 0);

		uint32_t zombie_bone_esp_thickness = 1u;

		float zombie_bone_esp_max_distance = 0.f;

		// player boxesp
		bool player_box_esp_active = true;
		bool player_box_esp_3d = false;

		color_t player_box_esp_color = color_t(0, 255, 0);

		uint32_t player_box_esp_thickness = 1u;

		// zombie boxesp
		bool zombie_box_esp_active = true;
		bool zombie_box_esp_3d = false;

		color_t zombie_box_esp_color = color_t(255, 127, 0);

		uint32_t zombie_box_esp_thickness = 1u;

		// player esp
		bool player_esp_distance = true;
		bool player_esp_healthbar = true;
		bool player_esp_info_text_shadow = true;

		color_t player_esp_info_text_color = color_t(255, 255, 255);
		color_t player_esp_info_text_shadow_color = color_t(0, 0, 0, 200);

		float player_esp_bbox_scale_width = 1.f;
		float player_esp_bbox_scale_height = 1.f;

		// zombie esp
		bool zombie_esp_distance = true;
		bool zombie_esp_info_text_shadow = true;

		color_t zombie_esp_info_text_color = color_t(255, 255, 255);
		color_t zombie_esp_info_text_shadow_color = color_t(0, 0, 0, 200);

		float zombie_esp_bbox_scale_width = 1.f;
		float zombie_esp_bbox_scale_height = 1.f;

		// misc
		bool misc_showframetime = true;
		bool misc_showfps = true;
		bool misc_esp_text_scaling = true;

		int32_t misc_target_fps = -1;
		int32_t misc_bones_target_fps = 60u;

		float misc_esp_min_text_scale = 1.f;
		float misc_esp_max_text_scale = 1.f;
		float misc_esp_text_scaling_factor = .25f;

		void create(external_cheat_interfaces::cfg_t* cfg)
		{
			
		}

	};

	// main.cpp
	extern config_t config;

}

#pragma pop_macro("min")
#pragma pop_macro("max")