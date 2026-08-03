#pragma once

// PastAware menu — the UI layer ported from the velocity menu and adapted:
// same look (sidebar, tabs, theme swatches, search), self-contained settings.

#include "pch.hpp"
#include "../external/xdraw/xui/xui.hpp"

namespace rendering {

	class menu
	{
	public:
		void initialize( );            // load svg/png textures, called once after xdraw init
		void draw( );                  // called every present frame
		void shutdown( ) const;        // restore mouse lock on unload

		void toggle( ) { this->m_open = !this->m_open; }
		[[nodiscard]] bool is_open( ) const { return this->m_open; }
		void apply_saved_cursor( );

		void draw_watermark( );
		void draw_keybinds( );

		enum class tab : int
		{
			ragebot,
			legitbot,
			player,
			world,
			skins,
			misc,
			config,
			count
		};

	private:
		void draw_side_bar( float h );
		void draw_top_bar( float w );
		void draw_theme_swatches( float sb_x, float avatar_y );
		void apply_theme_preset( int preset );
		void sync_theme_style( ) const;
		void draw_search_results( float x, float y, float w, float h );
		void rebuild_search_index( );
		void close_search( );
		void activate_search_result( std::size_t index );

		void draw_ragebot( float group_w ) const;
		void draw_legitbot( float group_w ) const;
		void draw_player( float group_w ) const;
		void draw_world( float group_w ) const;
		void draw_skins( float group_w ) const;
		void draw_misc( float group_w ) const;
		void draw_config( float group_w );

		bool m_open{ true };
		bool m_last_open{ true };
		float m_open_anim{ 1.0f };
		std::uint8_t m_saved_relative_mouse{ };
		bool m_has_saved_cursor{ };
		int m_saved_cursor_x{ };
		int m_saved_cursor_y{ };

		float m_x{ 100.0f };
		float m_y{ 100.0f };
		float m_w{ 700.0f };
		float m_h{ 450.0f };
		float m_body_x{ };
		float m_body_y{ };
		float m_body_w{ };
		float m_body_h{ };

		int m_tab{ };
		int m_subtab{ };
		int m_subtab_pill_tab{ -1 };
		float m_subtab_pill_x{ -1.0f };
		bool m_search_open{ };
		int m_theme_preset{ };
		std::string m_search_query{ };
		std::vector<std::size_t> m_search_visible_indices{ };

		struct search_entry
		{
			std::string name{ };
			std::string category{ };
			std::string name_lower{ };
			std::string category_lower{ };
			int tab{ };
			int subtab{ };
			int bind_key{ };
		};
		std::vector<search_entry> m_search_entries{ };

		struct textures
		{
			struct entry
			{
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> resource{ };
				int width{ };
				int height{ };
			};

			entry logo{ };
			entry user{ };
			entry tabs[ 7 ]{ };
			entry search{ };
			entry settings{ };
			entry cfg_folder_on{ };
			entry cfg_folder_off{ };
			entry cfg_cloud_on{ };
			entry cfg_cloud_off{ };
			entry cfg_plus{ };
		} m_textures{ };

		static constexpr auto k_max_subtabs{ 6 };

		struct subtab_info
		{
			const char* names[ k_max_subtabs ]{ };
			int count{ };
		};

		static constexpr subtab_info k_subtab_defs[ static_cast< int >( tab::count ) ]
		{
			{ { "pistols", "smgs", "rifles", "shotguns", "snipers", "lmgs" }, 6 },
			{ { "pistols", "smgs", "rifles", "shotguns", "snipers", "lmgs" }, 6 },
			{ { "enemies", "allies", "local" },                           3 },
			{ { "esp", "scene", "weather" },                              3 },
			{ { "guns", "knives", "gloves", "agents" },                   4 },
			{ { "main", "removals", "camera", "hud" },                    4 },
			{ { "general" },                                              1 }
		};
	};

	inline menu g_menu{ };

} // namespace rendering
