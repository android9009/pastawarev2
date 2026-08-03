#include "pch.hpp"

#include "menu.hpp"
#include "settings.hpp"
#include "menu_svgs.hpp"
#include "menu_assets.hpp"
#include "hooks.hpp"

namespace rendering {

	namespace detail {

		[[nodiscard]] static std::string to_lower_copy( std::string_view v )
		{
			std::string out( v );
			std::transform( out.begin( ), out.end( ), out.begin( ), [ ]( unsigned char c )
				{
					return static_cast< char >( std::tolower( c ) );
				} );
			return out;
		}

		// Maps a setting category to (tab, subtab) for the search feature.
		[[nodiscard]] static std::pair<int, int> map_category_to_tab( const std::string& category_lower )
		{
			if ( category_lower.find( "ragebot" ) != std::string::npos ) return { 0, 0 };
			if ( category_lower.find( "legitbot" ) != std::string::npos ) return { 1, 0 };

			if ( category_lower.find( "allies" ) != std::string::npos ) return { 2, 1 };
			if ( category_lower.find( "local" ) != std::string::npos ) return { 2, 2 };
			if ( category_lower.find( "enemies" ) != std::string::npos ) return { 2, 0 };

			if ( category_lower.find( "scene" ) != std::string::npos ) return { 3, 1 };
			if ( category_lower.find( "weather" ) != std::string::npos ) return { 3, 2 };
			if ( category_lower.find( "world" ) != std::string::npos || category_lower.find( "esp" ) != std::string::npos ) return { 3, 0 };

			if ( category_lower.find( "knives" ) != std::string::npos ) return { 4, 1 };
			if ( category_lower.find( "gloves" ) != std::string::npos ) return { 4, 2 };
			if ( category_lower.find( "agents" ) != std::string::npos ) return { 4, 3 };
			if ( category_lower.find( "skins" ) != std::string::npos ) return { 4, 0 };

			if ( category_lower.find( "removals" ) != std::string::npos ) return { 5, 1 };
			if ( category_lower.find( "camera" ) != std::string::npos ) return { 5, 2 };
			if ( category_lower.find( "hud" ) != std::string::npos ) return { 5, 3 };
			if ( category_lower.find( "main" ) != std::string::npos || category_lower.find( "misc" ) != std::string::npos ) return { 5, 0 };

			if ( category_lower.find( "config" ) != std::string::npos ) return { 6, 0 };

			return { 5, 0 };
		}

	} // namespace detail

	namespace tokens {

		inline xdraw::color col_accent{ 173, 192, 255, 255 };
		inline xdraw::color col_dark{ 17, 17, 17, 255 };
		inline xdraw::color col_text{ 221, 229, 255, 235 };
		inline xdraw::color col_text_dim{ 173, 192, 255, 82 };
		inline xdraw::color col_card{ 17, 17, 17, 82 };
		inline xdraw::color col_elevated{ 31, 31, 35, 118 };

		constexpr auto sidebar_w{ 42.0f };
		constexpr auto tab_icon_size{ 35.0f };
		constexpr auto subtab_bar_h{ 35.0f };
		constexpr auto gap{ 8.0f };
		constexpr auto card_rounding{ 10.0f };
		constexpr auto btn_rounding{ 8.0f };

	} // namespace tokens

	void menu::initialize( )
	{
		constexpr auto icon_target{ 16.0f };
		constexpr auto tab_target{ 22.0f };

		const char* const tab_svgs[ 7 ]
		{
			menu_svgs::svgs::tab_rage,
			menu_svgs::svgs::tab_legit,
			menu_svgs::svgs::tab_player,
			menu_svgs::svgs::tab_world,
			menu_svgs::svgs::tab_skins,
			menu_svgs::svgs::tab_misc,
			menu_svgs::svgs::tab_config,
		};

		this->m_textures.logo.resource = xdraw::load_svg( menu_svgs::svgs::logo, icon_target / 14.0f, &this->m_textures.logo.width, &this->m_textures.logo.height );
		this->m_textures.search.resource = xdraw::load_svg( menu_svgs::svgs::search, icon_target / 16.0f, &this->m_textures.search.width, &this->m_textures.search.height );
		this->m_textures.settings.resource = xdraw::load_svg( menu_svgs::svgs::settings, icon_target / 14.0f, &this->m_textures.settings.width, &this->m_textures.settings.height );

		for ( auto i = 0; i < 7; ++i )
		{
			this->m_textures.tabs[ i ].resource = xdraw::load_svg( tab_svgs[ i ], tab_target / 24.0f, &this->m_textures.tabs[ i ].width, &this->m_textures.tabs[ i ].height );
		}

		this->m_textures.cfg_folder_on.resource = xdraw::load_svg( menu_svgs::svgs::cfg_folder_black, 1.0f, &this->m_textures.cfg_folder_on.width, &this->m_textures.cfg_folder_on.height );
		this->m_textures.cfg_folder_off.resource = xdraw::load_svg( menu_svgs::svgs::cfg_folder_dim, 1.0f, &this->m_textures.cfg_folder_off.width, &this->m_textures.cfg_folder_off.height );
		this->m_textures.cfg_cloud_on.resource = xdraw::load_svg( menu_svgs::svgs::cfg_cloud_black, 1.0f, &this->m_textures.cfg_cloud_on.width, &this->m_textures.cfg_cloud_on.height );
		this->m_textures.cfg_cloud_off.resource = xdraw::load_svg( menu_svgs::svgs::cfg_cloud_dim, 1.0f, &this->m_textures.cfg_cloud_off.width, &this->m_textures.cfg_cloud_off.height );
		this->m_textures.cfg_plus.resource = xdraw::load_svg( menu_svgs::svgs::cfg_plus, 1.0f, &this->m_textures.cfg_plus.width, &this->m_textures.cfg_plus.height );

		this->m_textures.user.resource = xdraw::load_texture( std::span( reinterpret_cast< const std::byte* >( menu_assets::user ), sizeof( menu_assets::user ) ), &this->m_textures.user.width, &this->m_textures.user.height );

		this->rebuild_search_index( );
	}

	void menu::shutdown( ) const
	{
		if ( this->m_open )
		{
			hooks::set_relative_mouse( this->m_saved_relative_mouse != 0 );
		}
	}

	void menu::apply_saved_cursor( )
	{
		if ( !this->m_has_saved_cursor )
		{
			return;
		}

		const auto vx = GetSystemMetrics( SM_XVIRTUALSCREEN );
		const auto vy = GetSystemMetrics( SM_YVIRTUALSCREEN );
		const auto vw = GetSystemMetrics( SM_CXVIRTUALSCREEN );
		const auto vh = GetSystemMetrics( SM_CYVIRTUALSCREEN );

		const auto x = std::clamp( this->m_saved_cursor_x, vx, vx + std::max( 1, vw ) - 1 );
		const auto y = std::clamp( this->m_saved_cursor_y, vy, vy + std::max( 1, vh ) - 1 );

		SetCursorPos( x, y );
		SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
	}

	void menu::draw( )
	{
		if ( this->m_last_open != this->m_open )
		{
			if ( this->m_open )
			{
				this->m_saved_relative_mouse = hooks::relative_mouse( );
				hooks::set_relative_mouse( false );
				this->apply_saved_cursor( );
			}
			else
			{
				POINT pt{ };
				if ( GetCursorPos( &pt ) )
				{
					this->m_saved_cursor_x = pt.x;
					this->m_saved_cursor_y = pt.y;
					this->m_has_saved_cursor = true;
				}

				hooks::set_relative_mouse( this->m_saved_relative_mouse != 0 );
			}

			this->m_last_open = this->m_open;
		}

		xui::begin( );
		this->sync_theme_style( );
		{
			const auto dt = xdraw::delta_time( );
			const auto anim_speed = this->m_open ? 14.0f : 16.0f;
			const auto anim_target = this->m_open ? 1.0f : 0.0f;
			this->m_open_anim += ( anim_target - this->m_open_anim ) * std::min( anim_speed * dt, 1.0f );

			if ( this->m_open_anim < 0.01f && !this->m_open )
			{
				xui::end( );
				return;
			}

			const auto menu_reveal = xui::ease::out_cubic( this->m_open_anim );

			if ( !xui::begin_window( "##menu", this->m_x, this->m_y, this->m_w, this->m_h, false, 200.0f, 200.0f, menu_reveal ) )
			{
				return;
			}

			auto& dl = xui::draw::current( );
			const auto wx = this->m_x;
			const auto wy = this->m_y;
			const auto ww = this->m_w;
			const auto wh = this->m_h;

			const auto sb_x = wx + tokens::gap;
			const auto sb_y = wy + tokens::gap;
			const auto sb_w = tokens::sidebar_w;
			const auto sb_h = wh - tokens::gap * 2.0f;

			const auto logo_h = tokens::subtab_bar_h;
			dl.rect_filled( sb_x, sb_y, sb_w, logo_h, tokens::col_card, xdraw::corner_radius{ tokens::card_rounding } );

			const auto logo_pill_x = sb_x + 4.0f;
			const auto logo_pill_y = sb_y + 4.0f;
			const auto logo_pill_w = sb_w - 8.0f;
			const auto logo_pill_h = logo_h - 8.0f;
			dl.rect_filled( logo_pill_x, logo_pill_y, logo_pill_w, logo_pill_h, tokens::col_accent, xdraw::corner_radius{ tokens::btn_rounding } );

			const auto tabs_y = sb_y + logo_h + tokens::gap;
			const auto tabs_h = sb_h - logo_h - tokens::gap;
			dl.rect_filled( sb_x, tabs_y, sb_w, tabs_h, tokens::col_card, xdraw::corner_radius{ tokens::card_rounding } );

			this->draw_side_bar( tabs_h );

			const auto content_x = sb_x + sb_w + tokens::gap;
			const auto content_y = sb_y;
			const auto content_w = ww - tokens::gap * 2.0f - sb_w - tokens::gap;
			const auto content_h = sb_h;

			this->draw_top_bar( content_w );

			const auto body_y = content_y + tokens::subtab_bar_h + tokens::gap;
			const auto body_h = content_h - tokens::subtab_bar_h - tokens::gap;
			const auto col_w = ( content_w - tokens::gap ) * 0.5f;

			this->m_body_x = content_x;
			this->m_body_y = body_y;
			this->m_body_w = content_w;
			this->m_body_h = body_h;
			xui::layout::set_cursor( content_x - wx, body_y - wy );

			if ( this->m_search_open )
			{
				// While search is open, block the regular top-bar interactions in this frame.
				xui::ctx( ).inside_overlay = xui::fnv1a( "menu_search_mode" );
				this->draw_search_results( content_x, body_y, content_w, body_h );
				xui::ctx( ).inside_overlay = xui::null_id;

				xui::end_window( );
				xui::end( );
				return;
			}

			switch ( this->m_tab )
			{
			case 0: this->draw_ragebot( col_w ); break;
			case 1: this->draw_legitbot( col_w ); break;
			case 2: this->draw_player( col_w ); break;
			case 3: this->draw_world( col_w ); break;
			case 4: this->draw_skins( col_w ); break;
			case 5: this->draw_misc( col_w ); break;
			case 6: this->draw_config( col_w ); break;
			default: break;
			}

			xui::end_window( );
		}
		xui::end( );
	}

	void menu::draw_side_bar( float h )
	{
		auto& dl = xui::draw::current( );
		const auto& input = xui::ctx( ).input;

		const auto sb_x = this->m_x + tokens::gap;
		const auto logo_h = tokens::subtab_bar_h;
		const auto tabs_y = this->m_y + tokens::gap + logo_h + tokens::gap;

		{
			const auto pill_x = sb_x + 4.0f;
			const auto pill_y = this->m_y + tokens::gap + 4.0f;
			const auto pill_w = tokens::sidebar_w - 8.0f;
			const auto pill_h = logo_h - 8.0f;

			const auto lw = static_cast< float >( this->m_textures.logo.width );
			const auto lh = static_cast< float >( this->m_textures.logo.height );
			const auto lx = std::floor( pill_x + ( pill_w - lw ) * 0.5f );
			const auto ly = std::floor( pill_y + ( pill_h - lh ) * 0.5f );

			dl.image( lx, ly, lw, lh, this->m_textures.logo.resource.Get( ) );
		}

		constexpr auto tab_count{ 7 };
		const auto icon_pad{ 4.0f };
		const auto icon_stride = tokens::tab_icon_size + 4.0f;

		for ( auto i = 0; i < tab_count; ++i )
		{
			const auto ix = sb_x + ( tokens::sidebar_w - tokens::tab_icon_size ) * 0.5f;
			const auto iy = tabs_y + icon_pad + i * icon_stride;
			const auto btn = xui::rect{ ix, iy, tokens::tab_icon_size, tokens::tab_icon_size };

			const auto hovered = input.in_rect( btn );
			const auto is_active = ( this->m_tab == i );

			if ( !this->m_search_open && hovered && input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_tab = i;
				this->m_subtab = 0;
			}

			const auto hover_anim = xui::anim::lerp( xui::fnv1a( "sidebar" ) + i, hovered ? 1.0f : 0.0f, 12.0f );
			const auto active_anim = xui::anim::lerp( xui::fnv1a( "sidebar_active" ) + i, is_active ? 1.0f : 0.0f, 10.0f );

			if ( active_anim > 0.01f )
			{
				auto bg = tokens::col_accent;
				bg.a = static_cast< std::uint8_t >( bg.a * active_anim );
				dl.rect_filled( btn.x, btn.y, btn.w, btn.h, bg, xdraw::corner_radius{ tokens::btn_rounding } );
			}

			auto icon_col = xui::lerp( tokens::col_text_dim, tokens::col_accent, hover_anim );
			if ( is_active )
			{
				icon_col = xui::lerp( icon_col, tokens::col_dark, active_anim );
			}

			{
				const auto& tex = this->m_textures.tabs[ i ];
				const auto iw = static_cast< float >( tex.width );
				const auto ih = static_cast< float >( tex.height );
				const auto icon_x = std::floor( btn.x + ( btn.w - iw ) * 0.5f );
				const auto icon_y = std::floor( btn.y + ( btn.h - ih ) * 0.5f );

				dl.image( icon_x, icon_y, iw, ih, tex.resource.Get( ), icon_col );
			}
		}

		const auto avatar_y = tabs_y + h - tokens::tab_icon_size - icon_pad;
		const auto avatar_x = sb_x + ( tokens::sidebar_w - tokens::tab_icon_size ) * 0.5f;

		this->draw_theme_swatches( sb_x, avatar_y );

		dl.image( std::floor( avatar_x ), std::floor( avatar_y ), tokens::tab_icon_size, tokens::tab_icon_size, this->m_textures.user.resource.Get( ), xdraw::corner_radius{ 7.0f } );
	}

	void menu::draw_theme_swatches( float sb_x, float avatar_y )
	{
		if ( this->m_search_open )
		{
			return;
		}

		auto& dl = xui::draw::current( );
		const auto& input = xui::ctx( ).input;

		static constexpr xdraw::color k_preset_colors[ 5 ]
		{
			xdraw::color{ 173, 192, 255, 255 },
			xdraw::color{ 190, 164, 255, 255 },
			xdraw::color{ 122, 224, 181, 255 },
			xdraw::color{ 255, 171, 112, 255 },
			xdraw::color{ 210, 214, 224, 255 },
		};

		constexpr auto swatch_count{ 5 };
		constexpr auto dot_d{ 6.0f };
		constexpr auto dot_gap{ 3.0f };
		const auto block_h = swatch_count * dot_d + ( swatch_count - 1 ) * dot_gap;
		const auto swatch_y = avatar_y - block_h - 10.0f;
		const auto cx = sb_x + tokens::sidebar_w * 0.5f;

		for ( auto i = 0; i < swatch_count; ++i )
		{
			const auto dot_y = swatch_y + static_cast< float >( i ) * ( dot_d + dot_gap );
			const auto hit = xui::rect{ cx - dot_d * 0.5f - 2.0f, dot_y - 2.0f, dot_d + 4.0f, dot_d + 4.0f };
			const auto hovered = input.in_rect( hit );
			const auto is_active = ( this->m_theme_preset == i );
			const auto hover_anim = xui::anim::lerp( xui::fnv1a( "theme_swatch" ) + i, ( hovered || is_active ) ? 1.0f : 0.0f, 14.0f );
			const auto radius = dot_d * 0.5f + hover_anim * 0.75f;
			const auto cy = dot_y + dot_d * 0.5f;

			if ( is_active )
			{
				dl.circle_filled( cx, cy, radius + 1.5f, tokens::col_text.alpha( 200 ) );
			}

			dl.circle_filled( cx, cy, radius, k_preset_colors[ i ] );

			if ( !xui::ctx( ).overlay_blocking( ) && hovered && input.mouse_clicked )
			{
				this->apply_theme_preset( i );
			}
		}
	}

	void menu::apply_theme_preset( int preset )
	{
		this->m_theme_preset = std::clamp( preset, 0, 4 );

		switch ( this->m_theme_preset )
		{
		default:
		case 0:
			tokens::col_accent = xdraw::color{ 173, 192, 255, 255 };
			tokens::col_dark = xdraw::color{ 17, 17, 17, 255 };
			tokens::col_text = xdraw::color{ 221, 229, 255, 235 };
			tokens::col_text_dim = xdraw::color{ 173, 192, 255, 82 };
			tokens::col_card = xdraw::color{ 17, 17, 17, 82 };
			tokens::col_elevated = xdraw::color{ 31, 31, 35, 118 };
			break;
		case 1:
			tokens::col_accent = xdraw::color{ 190, 164, 255, 255 };
			tokens::col_dark = xdraw::color{ 18, 17, 23, 255 };
			tokens::col_text = xdraw::color{ 235, 229, 255, 235 };
			tokens::col_text_dim = xdraw::color{ 190, 164, 255, 88 };
			tokens::col_card = xdraw::color{ 18, 17, 23, 88 };
			tokens::col_elevated = xdraw::color{ 34, 31, 43, 125 };
			break;
		case 2:
			tokens::col_accent = xdraw::color{ 122, 224, 181, 255 };
			tokens::col_dark = xdraw::color{ 14, 20, 19, 255 };
			tokens::col_text = xdraw::color{ 222, 246, 238, 235 };
			tokens::col_text_dim = xdraw::color{ 122, 224, 181, 88 };
			tokens::col_card = xdraw::color{ 14, 20, 19, 88 };
			tokens::col_elevated = xdraw::color{ 25, 39, 35, 125 };
			break;
		case 3:
			tokens::col_accent = xdraw::color{ 255, 171, 112, 255 };
			tokens::col_dark = xdraw::color{ 22, 17, 15, 255 };
			tokens::col_text = xdraw::color{ 255, 232, 214, 235 };
			tokens::col_text_dim = xdraw::color{ 255, 171, 112, 88 };
			tokens::col_card = xdraw::color{ 22, 17, 15, 88 };
			tokens::col_elevated = xdraw::color{ 43, 32, 26, 125 };
			break;
		case 4:
			tokens::col_accent = xdraw::color{ 210, 214, 224, 255 };
			tokens::col_dark = xdraw::color{ 18, 18, 20, 255 };
			tokens::col_text = xdraw::color{ 232, 235, 242, 235 };
			tokens::col_text_dim = xdraw::color{ 210, 214, 224, 82 };
			tokens::col_card = xdraw::color{ 18, 18, 20, 92 };
			tokens::col_elevated = xdraw::color{ 35, 35, 39, 126 };
			break;
		}
	}

	void menu::sync_theme_style( ) const
	{
		auto& style = xui::ctx( ).style;
		style.window_bg = tokens::col_elevated.alpha( 175 );
		style.child_bg = tokens::col_card;
		style.checkbox_bg = tokens::col_card;
		style.checkbox_mark = tokens::col_accent;
		style.checkbox_mark_icon = tokens::col_dark;
		style.slider_track = tokens::col_card;
		style.slider_fill = tokens::col_accent;
		style.button_bg = tokens::col_card;
		style.button_hovered = tokens::col_card.alpha( 120 );
		style.button_active = tokens::col_accent;
		style.keybind_bg = tokens::col_card;
		style.keybind_waiting = tokens::col_accent;
		style.combo_bg = tokens::col_card;
		style.combo_arrow = tokens::col_text_dim;
		style.combo_hovered = tokens::col_card.alpha( 120 );
		style.combo_popup_bg = tokens::col_elevated.alpha( 170 );
		style.combo_popup_item_hovered = tokens::col_card.alpha( 125 );
		style.combo_popup_item_selected = tokens::col_accent.alpha( 42 );
		style.popup_bg = tokens::col_elevated.alpha( 170 );
		style.picker_bg = tokens::col_card;
		style.picker_popup_bg = tokens::col_elevated.alpha( 170 );
		style.text_input_bg = tokens::col_card;
		style.separator = tokens::col_accent.alpha( 30 );
		style.text = tokens::col_text;
		style.text_dim = tokens::col_text_dim;
		style.accent = tokens::col_accent;
	}

	void menu::draw_top_bar( float w )
	{
		auto& dl = xui::draw::current( );
		const auto& input = xui::ctx( ).input;

		const auto content_x = this->m_x + tokens::gap + tokens::sidebar_w + tokens::gap;
		const auto bar_y = this->m_y + tokens::gap;

		const auto& def = k_subtab_defs[ this->m_tab ];
		const auto subtab_count = def.count;

		const auto inner_pad{ 4.0f };
		const auto subtab_h = tokens::subtab_bar_h - inner_pad * 2.0f;
		const auto util_w = inner_pad + subtab_h + inner_pad;
		const auto search_anim = xui::anim::lerp( xui::fnv1a( "menu_topbar_search_anim" ), this->m_search_open ? 1.0f : 0.0f, 18.0f );
		const auto normal_interactive = search_anim < 0.03f;

		const auto subtabs_w = w - util_w - tokens::gap;
		const auto btn_w = ( subtabs_w - inner_pad * 2.0f ) / static_cast< float >( subtab_count );

		dl.rect_filled( content_x, bar_y, subtabs_w, tokens::subtab_bar_h, tokens::col_card, xdraw::corner_radius{ tokens::card_rounding } );
		dl.rect_filled( content_x + subtabs_w + tokens::gap, bar_y, util_w, tokens::subtab_bar_h, tokens::col_card, xdraw::corner_radius{ tokens::card_rounding } );

		const auto by = bar_y + ( tokens::subtab_bar_h - subtab_h ) * 0.5f;
		const auto pill_target_x = content_x + inner_pad + btn_w * static_cast< float >( this->m_subtab );

		if ( this->m_subtab_pill_tab != this->m_tab || this->m_subtab_pill_x < 0.0f )
		{
			this->m_subtab_pill_x = pill_target_x;
			this->m_subtab_pill_tab = this->m_tab;
		}
		else
		{
			const auto dt = xdraw::delta_time( );
			this->m_subtab_pill_x += ( pill_target_x - this->m_subtab_pill_x ) * std::min( 18.0f * dt, 1.0f );
		}

		if ( subtab_count > 0 )
		{
			dl.rect_filled( this->m_subtab_pill_x, by, btn_w, subtab_h, tokens::col_accent, xdraw::corner_radius{ tokens::btn_rounding } );
		}

		for ( auto i = 0; i < subtab_count; ++i )
		{
			const auto bx = content_x + inner_pad + btn_w * i;
			const auto btn = xui::rect{ bx, by, btn_w, subtab_h };

			const auto hovered = input.in_rect( btn );
			const auto is_active = ( this->m_subtab == i );

			if ( normal_interactive && hovered && input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_subtab = i;
			}

			const auto active_anim = xui::anim::lerp( xui::fnv1a( "subtab_text" ) + i, is_active ? 1.0f : 0.0f, 12.0f );
			const auto hover_anim = xui::anim::lerp( xui::fnv1a( "subtab_hover" ) + i, hovered && !is_active ? 1.0f : 0.0f, 14.0f );

			if ( hover_anim > 0.01f )
			{
				dl.rect_filled( btn.x, btn.y, btn.w, btn.h, tokens::col_elevated.alpha( static_cast< std::uint8_t >( 90.0f * hover_anim ) ), xdraw::corner_radius{ tokens::btn_rounding } );
			}

			const auto [tw, th] = xdraw::measure_text( def.names[ i ] );
			const auto tx = std::floor( btn.x + ( btn.w - tw ) * 0.5f );
			const auto ty = std::floor( btn.y + ( btn.h - th ) * 0.5f );

			auto text_col = xui::lerp( tokens::col_text_dim, tokens::col_dark, active_anim );
			text_col = xui::lerp( text_col, tokens::col_text, hover_anim * 0.35f );
			dl.text( tx, ty, def.names[ i ], text_col );
		}

		const auto util_x = content_x + subtabs_w + tokens::gap;

		{
			const auto bx = util_x + inner_pad;
			const auto by_ = bar_y + ( tokens::subtab_bar_h - subtab_h ) * 0.5f;
			const auto btn = xui::rect{ bx, by_, subtab_h, subtab_h };
			const auto hovered = input.in_rect( btn );
			const auto hover_anim = xui::anim::lerp( xui::fnv1a( "menu_search_btn" ), hovered ? 1.0f : 0.0f, 14.0f );
			if ( hover_anim > 0.01f )
			{
				dl.rect_filled( bx, by_, subtab_h, subtab_h, tokens::col_card.alpha( static_cast< std::uint8_t >( 120.0f * hover_anim ) ), xdraw::corner_radius{ tokens::btn_rounding } );
			}

			if ( this->m_textures.search.resource )
			{
				const auto iw = static_cast< float >( this->m_textures.search.width );
				const auto ih = static_cast< float >( this->m_textures.search.height );
				dl.image( std::floor( bx + ( subtab_h - iw ) * 0.5f ), std::floor( by_ + ( subtab_h - ih ) * 0.5f ), iw, ih, this->m_textures.search.resource.Get( ), xui::lerp( tokens::col_text_dim, tokens::col_text, hover_anim ) );
			}

			if ( normal_interactive && hovered && input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
			{
				this->m_search_open = true;
				this->m_search_query.clear( );
				this->rebuild_search_index( );
				return;
			}
		}

		if ( search_anim > 0.01f )
		{
			const auto t = search_anim * search_anim * ( 3.0f - 2.0f * search_anim );
			const auto panel_w = std::lerp( util_w, w, t );
			const auto panel_x = content_x + w - panel_w;
			const auto panel_y = bar_y;
			const auto panel_h = tokens::subtab_bar_h;
			const auto panel_rounding = tokens::card_rounding;
			const auto panel_alpha = static_cast< std::uint8_t >( std::clamp( 190.0f + 55.0f * t, 0.0f, 245.0f ) );

			dl.rect_filled( panel_x, panel_y, panel_w, panel_h, tokens::col_dark.alpha( static_cast< std::uint8_t >( 225.0f * t ) ), xdraw::corner_radius{ panel_rounding } );
			dl.rect_filled_blurred( panel_x, panel_y, panel_w, panel_h, xdraw::corner_radius{ panel_rounding }, xdraw::color{ 255, 255, 255, static_cast< std::uint8_t >( 130.0f * t ) } );
			dl.rect_filled( panel_x, panel_y, panel_w, panel_h, tokens::col_elevated.alpha( panel_alpha ), xdraw::corner_radius{ panel_rounding } );

			const auto close_w = subtab_h;
			const auto close_x = panel_x + panel_w - inner_pad - close_w;
			const auto close_y = bar_y + ( tokens::subtab_bar_h - subtab_h ) * 0.5f;
			const auto close_rect = xui::rect{ close_x, close_y, close_w, subtab_h };
			const auto close_hovered = input.in_rect( close_rect );
			const auto close_anim = xui::anim::lerp( xui::fnv1a( "menu_search_close" ), close_hovered ? 1.0f : 0.0f, 14.0f );
			const auto close_bg = xui::lerp( tokens::col_card.alpha( 0 ), tokens::col_accent.alpha( 80 ), close_anim );

			if ( panel_w > util_w + 24.0f )
			{
				const auto input_pad = inner_pad + 8.0f;
				const auto input_x = panel_x + input_pad;
				const auto input_y = bar_y + ( tokens::subtab_bar_h - subtab_h ) * 0.5f;
				const auto input_w = std::max( 24.0f, panel_w - input_pad - close_w - inner_pad * 2.0f );

				dl.push_clip( panel_x + inner_pad, panel_y, panel_w - inner_pad * 2.0f, panel_h );

				if ( auto* win = xui::layout::current_window( ) )
				{
					const auto saved_bounds = win->bounds;
					const auto saved_cursor_x = win->cursor_x;
					const auto saved_cursor_y = win->cursor_y;
					const auto saved_line_h = win->line_h;

					win->bounds.w = input_x - win->bounds.x + input_w + xui::ctx( ).style.window_pad_x;
					xui::layout::set_cursor( input_x - this->m_x, input_y - this->m_y );

					xui::push_style_var( xui::style_var::text_input_h, subtab_h );
					xui::push_style_var( xui::style_var::text_input_rounding, tokens::btn_rounding );
					xui::push_style_color( xui::style_col::text_input_bg, tokens::col_card.alpha( 0 ) );
					xui::push_style_color( xui::style_col::text_input_border, tokens::col_card.alpha( 0 ) );
					xui::text_input( "##menu_search_input", this->m_search_query, 96, "find a setting..." );
					xui::pop_style_color( 2 );
					xui::pop_style_var( 2 );

					win->bounds = saved_bounds;
					win->cursor_x = saved_cursor_x;
					win->cursor_y = saved_cursor_y;
					win->line_h = saved_line_h;
				}

				dl.pop_clip( );
			}

			dl.rect_filled( close_x, close_y, close_w, subtab_h, close_bg, xdraw::corner_radius{ tokens::btn_rounding } );

			const auto cx = close_x + close_w * 0.5f;
			const auto cy = close_y + subtab_h * 0.5f;
			const auto span = 4.0f;
			auto x_col = xui::lerp( tokens::col_text_dim.alpha( static_cast< std::uint8_t >( 180.0f * t ) ), tokens::col_text, std::max( close_anim, t ) );
			dl.line( cx - span, cy - span, cx + span, cy + span, x_col, 1.3f );
			dl.line( cx - span, cy + span, cx + span, cy - span, x_col, 1.3f );

			if ( close_hovered && input.mouse_clicked )
			{
				this->close_search( );
			}

			for ( const auto vk : input.key_presses( ) )
			{
				if ( vk == VK_ESCAPE )
				{
					this->close_search( );
					break;
				}
			}
		}
	}

	void menu::rebuild_search_index( )
	{
		this->m_search_entries.clear( );

		const auto all_settings = xui::binds::all( );
		this->m_search_entries.reserve( all_settings.size( ) );

		for ( const auto* setting : all_settings )
		{
			if ( !setting || setting->name.empty( ) )
			{
				continue;
			}

			search_entry entry{ };
			entry.name = setting->name;
			entry.category = setting->category;
			entry.name_lower = detail::to_lower_copy( entry.name );
			entry.category_lower = detail::to_lower_copy( entry.category );
			std::tie( entry.tab, entry.subtab ) = detail::map_category_to_tab( entry.category_lower );
			entry.bind_key = setting->bind.key;

			this->m_search_entries.emplace_back( std::move( entry ) );
		}
	}

	void menu::close_search( )
	{
		this->m_search_open = false;
		this->m_search_query.clear( );
		this->m_search_visible_indices.clear( );
	}

	void menu::activate_search_result( const std::size_t index )
	{
		if ( index >= this->m_search_visible_indices.size( ) )
		{
			return;
		}

		const auto& chosen = this->m_search_entries[ this->m_search_visible_indices[ index ] ];
		this->m_tab = std::clamp( chosen.tab, 0, static_cast< int >( tab::count ) - 1 );
		this->m_subtab = std::clamp( chosen.subtab, 0, k_subtab_defs[ this->m_tab ].count - 1 );
		xui::set_highlight_target( chosen.name, 1.2f );
		this->close_search( );
	}

	void menu::draw_search_results( float x, float y, float w, float h )
	{
		xui::layout::set_cursor( x - this->m_x, y - this->m_y );
		this->m_search_visible_indices.clear( );

		auto query_lower = detail::to_lower_copy( this->m_search_query );
		for ( std::size_t i = 0; i < this->m_search_entries.size( ); ++i )
		{
			const auto& it = this->m_search_entries[ i ];
			if ( query_lower.empty( ) || it.name_lower.find( query_lower ) != std::string::npos || it.category_lower.find( query_lower ) != std::string::npos )
			{
				this->m_search_visible_indices.emplace_back( i );
				if ( this->m_search_visible_indices.size( ) >= 50 )
				{
					break;
				}
			}
		}

		if ( xui::begin_child( "##menu_search_results", w, h, true ) )
		{
			for ( std::size_t i = 0; i < this->m_search_visible_indices.size( ); ++i )
			{
				const auto& item = this->m_search_entries[ this->m_search_visible_indices[ i ] ];

				const auto avail_w = xui::layout::avail( ).first;
				const auto row_h = 44.0f;
				const auto row = xui::layout::item( avail_w, row_h );
				const auto hovered = xui::ctx( ).input.in_rect( row );
				const auto row_anim = xui::anim::lerp( xui::fnv1a( "search_row" ) + static_cast< std::uintptr_t >( i ), hovered ? 1.0f : 0.0f, 12.0f );

				auto& dl = xui::draw::current( );
				const auto row_bg = xui::lerp( tokens::col_card, tokens::col_accent.alpha( 90 ), row_anim * 0.35f );
				dl.rect_filled( row.x, row.y, row.w, row.h, row_bg, xdraw::corner_radius{ 8.0f } );

				const auto label_col = xui::lerp( tokens::col_text, tokens::col_dark, row_anim * 0.2f );
				const auto sub_col = tokens::col_text_dim;
				const auto name_th = xdraw::measure_text( item.name ).second;
				const auto subtitle = item.category.empty( ) ? "misc" : item.category.c_str( );
				const auto sub_th = xdraw::measure_text( subtitle ).second;
				const auto block_h = name_th + 3.0f + sub_th;
				const auto text_start_y = row.y + ( row.h - block_h ) * 0.5f;
				const auto name_y = text_start_y;
				dl.text( row.x + 8.0f, name_y, item.name, label_col );

				const auto sub_y = name_y + name_th + 3.0f;
				dl.text( row.x + 8.0f, sub_y, subtitle, sub_col.alpha( 170 ) );

				if ( item.bind_key != 0 )
				{
					const auto key = xui::vk_name( item.bind_key );
					const auto [kw, kh] = xdraw::measure_text( key );
					const auto bh = 20.0f;
					const auto bw = kw + 16.0f;
					const auto bx = row.right( ) - bw - 6.0f;
					const auto by_ = row.y + ( row.h - bh ) * 0.5f;
					dl.rect_filled( bx, by_, bw, bh, tokens::col_card, xdraw::corner_radius{ 5.0f } );
					dl.text( bx + 8.0f, by_ + ( bh - kh ) * 0.5f, key, tokens::col_text_dim );
				}

				if ( hovered && xui::ctx( ).input.mouse_clicked && !xui::ctx( ).overlay_blocking( ) )
				{
					this->activate_search_result( i );
					break;
				}
			}

			xui::end_child( );
		}

		if ( !this->m_search_visible_indices.empty( ) )
		{
			for ( const auto vk : xui::ctx( ).input.key_presses( ) )
			{
				if ( vk == VK_RETURN )
				{
					this->activate_search_result( 0 );
					break;
				}
			}
		}
	}

	// ── watermark ──────────────────────────────────────────────────────────
	void menu::draw_watermark( )
	{
		if ( !settings::misc::watermark.value )
		{
			return;
		}

		auto& dl = xdraw::get( );
		const auto& s = xui::ctx( ).style;

		const auto [screen_w, screen_h] = xdraw::viewport_size( );
		const auto framerate = xdraw::framerate( );

		constexpr auto h{ 24.0f };
		constexpr auto margin{ 10.0f };
		constexpr auto r{ 8.0f };
		constexpr auto inner_r{ 6.0f };
		constexpr auto inner_pad{ 2.0f };
		constexpr auto text_pad_x{ 8.0f };
		constexpr auto text_nudge{ 0.5f };
		constexpr auto section_spacing{ 2.0f };
		constexpr auto logo_icon_size{ 12.0f };
		constexpr auto logo_icon_pad{ 7.0f };

		// ── time ────────────────────────────────────────────────────────────
		SYSTEMTIME st{ };
		GetLocalTime( &st );
		char time_buf[ 8 ]{ };
		std::snprintf( time_buf, sizeof( time_buf ), "%02d:%02d", st.wHour, st.wMinute );

		// ── fps ─────────────────────────────────────────────────────────────
		static auto smoothed_fps{ 0.0f };
		if ( smoothed_fps == 0.0f ) smoothed_fps = framerate;
		smoothed_fps += ( framerate - smoothed_fps ) * std::min( 2.0f * xdraw::delta_time( ), 1.0f );
		char fps_val[ 8 ]{ };
		std::snprintf( fps_val, sizeof( fps_val ), "%.0f", smoothed_fps );

		// ── logo ────────────────────────────────────────────────────────────
		const auto logo_scale = logo_icon_size / 12.0f;
		static auto logo_w = 0, logo_h = 0;
		static const auto logo = xdraw::load_svg( R"(<svg width="15" height="12" viewBox="0 0 15 12" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M0.131688 9.02626L6.40009 0.551371C6.94385 -0.18379 8.07861 -0.18379 8.62237 0.551371L14.8681 8.99564C15.2003 9.44476 14.8666 10.0674 14.2937 10.0674H12.9205C12.5679 10.0674 12.2512 9.86022 12.1214 9.54481L10.2638 5.0302C10.1631 4.78558 9.91739 4.62489 9.64393 4.62489C9.52346 4.62489 9.43618 4.73535 9.46834 4.84701L11.2808 11.1405C11.4053 11.5727 11.0674 12 10.6014 12H9.36667C9.09606 12 8.86578 11.8102 8.82422 11.5529L7.71627 3.99646C7.68733 3.81739 7.36739 3.82052 7.33103 3.99836L5.84387 11.5738C5.79319 11.8214 5.56756 12 5.30526 12H4.07334C3.594 12 3.25442 11.5497 3.40311 11.1112L5.4932 4.94752C5.54344 4.79932 5.42867 4.64708 5.26665 4.64708H5.22153C4.95747 4.64708 4.71827 4.79707 4.61165 5.02955L2.5027 9.62798C2.36225 9.93422 2.04374 10.1288 1.69595 10.1208L0.689398 10.0978C0.124293 10.0848 -0.195983 9.46937 0.131756 9.02626H0.131688Z" fill="#111111"/></svg>)", logo_scale, &logo_w, &logo_h );

		const auto inner_h = h - inner_pad * 2.0f;
		const auto logo_draw_w = static_cast< float >( logo_w );

		// ── measure text ─────────────────────────────────────────────────────
		const auto [name_tw, name_th] = xdraw::measure_text( "pastaware" );
		const auto [fps_vw, fps_vh] = xdraw::measure_text( fps_val );
		const auto fps_uh = xdraw::measure_text( " fps" ).second;
		const auto time_th = xdraw::measure_text( time_buf ).second;

		// ── pill widths ──────────────────────────────────────────────────────
		const auto logo_pill_w = logo_icon_pad + logo_draw_w + logo_icon_pad + name_tw + text_pad_x;
		const auto fps_pill_w = fps_vw + xdraw::measure_text( " fps" ).first + text_pad_x * 2.0f;
		const auto time_pill_w = xdraw::measure_text( time_buf ).first + text_pad_x * 2.0f;

		// ── total width ──────────────────────────────────────────────────────
		float target_w = inner_pad + logo_pill_w + section_spacing;
		if ( settings::misc::show_fps.value ) target_w += fps_pill_w + section_spacing;
		if ( settings::misc::show_time.value ) target_w += time_pill_w + section_spacing;
		target_w = target_w - section_spacing + inner_pad;

		static auto smoothed_w{ 0.0f };
		if ( smoothed_w == 0.0f ) smoothed_w = target_w;
		smoothed_w += ( target_w - smoothed_w ) * std::min( 8.0f * xdraw::delta_time( ), 1.0f );

		const auto w = smoothed_w;
		const auto x = static_cast< float >( screen_w ) - w - margin;
		const auto y = margin;

		dl.rect_filled_blurred( x, y, w, h, xdraw::corner_radius{ r } );
		dl.rect_filled( x, y, w, h, s.window_bg, xdraw::corner_radius{ r } );

		auto cx = x + inner_pad;

		auto draw_split_pill = [ & ]( const char* value, float vw, float vh, const char* unit, float uh, float pill_w )
			{
				dl.rect_filled( cx, y + inner_pad, pill_w, inner_h, s.child_bg, xdraw::corner_radius{ inner_r } );
				dl.text( cx + text_pad_x, y + ( h - vh ) * 0.5f + text_nudge, value, s.accent );
				dl.text( cx + text_pad_x + vw, y + ( h - uh ) * 0.5f + text_nudge, unit, s.text_dim );
				cx += pill_w + section_spacing;
			};

		auto draw_pill = [ & ]( const char* text, float th, float pill_w )
			{
				dl.rect_filled( cx, y + inner_pad, pill_w, inner_h, s.child_bg, xdraw::corner_radius{ inner_r } );
				dl.text( cx + text_pad_x, y + ( h - th ) * 0.5f + text_nudge, text, s.accent );
				cx += pill_w + section_spacing;
			};

		// logo pill (always shown)
		dl.rect_filled( cx, y + inner_pad, logo_pill_w, inner_h, s.accent, xdraw::corner_radius{ inner_r } );
		if ( logo )
		{
			dl.image( cx + logo_icon_pad, y + ( h - static_cast< float >( logo_h ) ) * 0.5f, static_cast< float >( logo_w ), static_cast< float >( logo_h ), logo.Get( ), s.checkbox_mark_icon );
		}
		dl.text( cx + logo_icon_pad + logo_draw_w + logo_icon_pad, y + ( h - name_th ) * 0.5f + text_nudge, "pastaware", s.checkbox_mark_icon );
		cx += logo_pill_w + section_spacing;

		if ( settings::misc::show_fps.value ) draw_split_pill( fps_val, fps_vw, fps_vh, " fps", fps_uh, fps_pill_w );
		if ( settings::misc::show_time.value ) draw_pill( time_buf, time_th, time_pill_w );
	}

	// ── active keybinds indicator ───────────────────────────────────────────
	void menu::draw_keybinds( )
	{
		struct row_t
		{
			std::string name;
			int key;
		};

		std::vector<row_t> rows;
		for ( const auto* setting : xui::binds::all( ) )
		{
			if ( !setting || setting->bind.key == 0 || !setting->bind.active || setting->name.empty( ) )
			{
				continue;
			}
			rows.push_back( { setting->name, setting->bind.key } );
		}

		if ( rows.empty( ) )
		{
			return;
		}

		const auto& s = xui::ctx( ).style;
		const auto [screen_w, screen_h] = xdraw::viewport_size( );

		constexpr auto margin{ 10.0f };
		constexpr auto row_h{ 21.0f };
		constexpr auto row_spacing{ 3.0f };
		constexpr auto header_h{ 24.0f };
		constexpr auto r{ 8.0f };
		constexpr auto inner_r{ 6.0f };
		constexpr auto inner_pad{ 2.0f };
		constexpr auto text_pad_x{ 8.0f };
		constexpr auto text_nudge{ 0.5f };
		constexpr auto icon_size{ 20.0f };

		// container anchored top-right, below the watermark
		const auto base_y = margin + 24.0f + 8.0f;
		const auto inner_h = row_h - inner_pad * 2.0f;

		const auto [header_tw, header_th] = xdraw::measure_text( "keybinds" );
		const auto header_w = inner_pad + icon_size + inner_pad + header_tw + text_pad_x * 2.0f + inner_pad;

		const auto w = std::max( header_w, 150.0f );
		const auto x = static_cast< float >( screen_w ) - w - margin;

		auto& dl = xdraw::get( );

		// header
		dl.rect_filled( x, base_y, header_w, header_h, s.window_bg, xdraw::corner_radius{ r } );
		dl.rect_filled( x + inner_pad, base_y + inner_pad, icon_size, header_h - inner_pad * 2.0f, s.accent, xdraw::corner_radius{ inner_r } );
		const auto htx = x + inner_pad + icon_size + inner_pad;
		dl.rect_filled( htx, base_y + inner_pad, header_tw + text_pad_x * 2.0f, header_h - inner_pad * 2.0f, s.child_bg, xdraw::corner_radius{ inner_r } );
		dl.text( htx + text_pad_x, base_y + ( header_h - header_th ) * 0.5f + text_nudge, "keybinds", s.accent );

		// rows
		auto row_y = base_y + header_h + row_spacing;
		for ( const auto& row : rows )
		{
			const auto [nw, nh] = xdraw::measure_text( row.name );
			const auto key = xui::vk_name( row.key );
			const auto [kw, kh] = xdraw::measure_text( key );
			const auto name_pill_w = nw + text_pad_x * 2.0f;
			const auto key_pill_w = kw + text_pad_x * 2.0f;
			const auto row_w = inner_pad + name_pill_w + inner_pad + key_pill_w + inner_pad;

			dl.rect_filled( x, row_y, row_w, row_h, s.window_bg, xdraw::corner_radius{ r } );
			dl.rect_filled( x + inner_pad, row_y + inner_pad, name_pill_w, inner_h, s.child_bg, xdraw::corner_radius{ inner_r } );
			dl.text( x + inner_pad + text_pad_x, row_y + ( row_h - nh ) * 0.5f + text_nudge, row.name, s.text_dim );

			const auto kpx = x + inner_pad + name_pill_w + inner_pad;
			dl.rect_filled( kpx, row_y + inner_pad, key_pill_w, inner_h, s.accent, xdraw::corner_radius{ inner_r } );
			dl.text( kpx + text_pad_x, row_y + ( row_h - kh ) * 0.5f + text_nudge, key, s.checkbox_mark_icon );

			row_y += row_h + row_spacing;
		}
	}

	// ── tabs ───────────────────────────────────────────────────────────────

	void menu::draw_ragebot( float group_w ) const
	{
		using namespace settings::ragebot;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		const auto weapon_idx = std::clamp( this->m_subtab, 0, 5 );
		auto& g = groups[ weapon_idx ];

		// give the per-weapon settings stable names/categories for search & binds
		const auto wname = weapon_names[ weapon_idx ];
		auto tag_weapon_group = [ & ]( xui::setting& s, const char* name )
			{
				s.name = name;
				s.category = "ragebot";
			};
		tag_weapon_group( g.force_shot, "force shot" );
		tag_weapon_group( g.body_aim, "body aim" );
		tag_weapon_group( g.no_spread, "no spread" );
		tag_weapon_group( g.quick_stop, "quick stop" );

		if ( xui::begin_child( "##rage_global", group_w ) )
		{
			xui::checkbox( "enabled##rage", enabled );
			xui::layout::separator( );

			xui::checkbox( "silent", silent );
			xui::checkbox( "auto scope", auto_scope );
			xui::checkbox( "auto stop", auto_stop );
			xui::checkbox( "prefer lethal", prefer_lethal );
			xui::checkbox( "zeusbot", zeusbot );
			xui::checkbox( "knifebot", knifebot );
			xui::checkbox( "revolver", revolvo );

			xui::layout::separator( );

			xui::combo( "priority hitbox##rage", hitbox, hitbox_names, 5 );
			xui::slider_float( "fov##rage", fov, 0.0f, 180.0f, "%.0f" );
			xui::slider_float( "min damage##rage", min_damage, 1.0f, 120.0f, "%.0f" );
			xui::slider_float( "hit chance##rage", hit_chance, 0.0f, 100.0f, "%.0f" );

			xui::end_child( );
		}

		xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

		if ( xui::begin_child( "##rage_weapon", group_w ) )
		{
			xui::text( wname, tokens::col_text_dim );

			xui::checkbox( "force shot", g.force_shot );
			xui::checkbox( "body aim", g.body_aim );
			xui::checkbox( "no spread", g.no_spread );
			xui::checkbox( "quick stop", g.quick_stop );

			xui::layout::separator( );

			xui::slider_float( "min damage##weapon", g.min_damage, 1.0f, 120.0f, "%.0f" );
			xui::slider_float( "hit chance##weapon", g.hit_chance, 0.0f, 100.0f, "%.0f" );

			xui::end_child( );
		}
	}

	void menu::draw_legitbot( float group_w ) const
	{
		using namespace settings::legitbot;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( xui::begin_child( "##legit_global", group_w ) )
		{
			xui::checkbox( "enabled##legit", enabled );
			xui::layout::separator( );

			xui::combo( "hitbox##legit", hitbox, hitbox_names, 5 );
			xui::slider_float( "fov##legit", fov, 0.0f, 30.0f, "%.1f" );
			xui::slider_float( "smoothing##legit", smooth, 1.0f, 30.0f, "%.0f" );

			xui::end_child( );
		}

		xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

		if ( xui::begin_child( "##legit_advanced", group_w ) )
		{
			xui::checkbox( "visible check", visible_check );
			xui::checkbox( "recoil control", rcs );
			xui::slider_float( "rcs amount", rcs_amt, 0.0f, 100.0f, "%.0f" );
			xui::checkbox( "smoothing", smooth_enabled );

			xui::layout::separator( );

			xui::checkbox( "backtrack", backtrack );
			xui::slider_float( "backtrack ms", backtrack_ms, 1.0f, 400.0f, "%.0f" );

			xui::end_child( );
		}
	}

	void menu::draw_player( float group_w ) const
	{
		using namespace settings::player;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( this->m_subtab <= 1 )
		{
			auto& ov = m_overlay[ this->m_subtab ];
			const auto category = ( this->m_subtab == 0 ) ? "enemies" : "allies";

			ov.m_box.style = std::clamp( ov.m_box.style, 0, 1 );
			ov.m_health.position = std::clamp( ov.m_health.position, 0, 2 );
			ov.m_ammo.position = std::clamp( ov.m_ammo.position, 0, 2 );

			auto tag = [ & ]( xui::setting& s, const char* name )
				{
					s.name = name;
					s.category = category;
				};
			tag( ov.m_box.enabled, "bounding box" );
			tag( ov.m_box.fill, "box fill" );
			tag( ov.m_box.outline, "box outline" );
			tag( ov.m_health.enabled, "health bar" );
			tag( ov.m_health.gradient, "health gradient" );
			tag( ov.m_health.show_value, "health value" );
			tag( ov.m_ammo.enabled, "ammo bar" );
			tag( ov.m_name.enabled, "name" );
			tag( ov.m_weapon.enabled, "weapon" );
			tag( ov.m_skeleton.enabled, "skeleton" );
			tag( ov.m_glow.enabled, "glow" );
			tag( ov.m_chams.enabled, "chams" );
			tag( ov.m_oof.enabled, "oof arrows" );
			tag( ov.m_flags.enabled, "info flags" );
			tag( ov.m_radar.enabled, "radar" );

			if ( xui::begin_child( "##player_esp", group_w ) )
			{
				xui::checkbox( "esp overlay", ov.enabled );

				xui::checkbox( "bounding box", ov.m_box.enabled );
				if ( xui::begin_popup( "##box_popup", 220.0f ) )
				{
					xui::combo( "style##box", ov.m_box.style, box_styles, 2 );
					xui::checkbox( "fill##box", ov.m_box.fill );
					xui::checkbox( "outline##box", ov.m_box.outline );
					xui::slider_float( "corner length", ov.m_box.corner_length, 2.0f, 20.0f, "%.0f" );
					xui::end_popup( );
				}

				xui::checkbox( "name", ov.m_name.enabled );
				if ( xui::begin_popup( "##name_popup", 220.0f ) )
				{
					xui::color_picker( "color##name", ov.m_name.color );
					xui::end_popup( );
				}

				xui::checkbox( "weapon", ov.m_weapon.enabled );
				if ( xui::begin_popup( "##weapon_popup", 220.0f ) )
				{
					xui::color_picker( "color##weapon", ov.m_weapon.color );
					xui::end_popup( );
				}

				xui::checkbox( "skeleton", ov.m_skeleton.enabled );
				if ( xui::begin_popup( "##skeleton_popup", 220.0f ) )
				{
					xui::color_picker( "color##skeleton", ov.m_skeleton.color );
					xui::end_popup( );
				}

				xui::checkbox( "health bar", ov.m_health.enabled );
				if ( xui::begin_popup( "##health_popup", 220.0f ) )
				{
					xui::combo( "position##hp", ov.m_health.position, bar_positions, 3 );
					xui::checkbox( "gradient##hp", ov.m_health.gradient );
					xui::checkbox( "show value##hp", ov.m_health.show_value );
					xui::end_popup( );
				}

				xui::checkbox( "ammo bar", ov.m_ammo.enabled );
				if ( xui::begin_popup( "##ammo_popup", 220.0f ) )
				{
					xui::combo( "position##ammo", ov.m_ammo.position, bar_positions, 3 );
					xui::end_popup( );
				}

				xui::end_child( );
			}

			xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

			if ( xui::begin_child( "##player_extra", group_w ) )
			{
				xui::checkbox( "glow", ov.m_glow.enabled );
				if ( xui::begin_popup( "##glow_popup", 220.0f ) )
				{
					xui::color_picker( "color##glow", ov.m_glow.color );
					xui::end_popup( );
				}

				xui::checkbox( "chams", ov.m_chams.enabled );
				if ( xui::begin_popup( "##chams_popup", 220.0f ) )
				{
					xui::color_picker( "color##chams", ov.m_chams.color );
					xui::end_popup( );
				}

				xui::checkbox( "oof arrows", ov.m_oof.enabled );
				if ( xui::begin_popup( "##oof_popup", 220.0f ) )
				{
					xui::color_picker( "color##oof", ov.m_oof.color );
					xui::end_popup( );
				}

				xui::checkbox( "info flags", ov.m_flags.enabled );
				if ( xui::begin_popup( "##flags_popup", 220.0f ) )
				{
					xui::multicombo( "flags##mc", ov.m_flags.flags, flag_names, 8 );
					xui::end_popup( );
				}

				xui::checkbox( "radar", ov.m_radar.enabled );
				if ( xui::begin_popup( "##radar_popup", 220.0f ) )
				{
					xui::slider_float( "range##radar", ov.m_radar.range, 100.0f, 1500.0f, "%.0f" );
					xui::end_popup( );
				}

				xui::end_child( );
			}
		}
		else
		{
			// local player
			if ( xui::begin_child( "##player_local", group_w ) )
			{
				xui::checkbox( "thirdperson", settings::misc::thirdperson );
				xui::checkbox( "camera smoothing", settings::misc::camera_smoothing );
				xui::layout::separator( );
				xui::slider_float( "fov##local", settings::misc::fov, 80.0f, 140.0f, "%.0f" );
				xui::slider_float( "viewmodel fov##local", settings::misc::viewmodel_fov, 50.0f, 120.0f, "%.0f" );

				xui::end_child( );
			}

			xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

			if ( xui::begin_child( "##player_local_extra", group_w ) )
			{
				xui::checkbox( "spectator list", settings::world::spectator_list );
				xui::checkbox( "local glow", settings::player::m_overlay[ 1 ].m_glow.enabled );
				xui::checkbox( "local chams", settings::player::m_overlay[ 1 ].m_chams.enabled );

				xui::end_child( );
			}
		}
	}

	void menu::draw_world( float group_w ) const
	{
		using namespace settings::world;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( this->m_subtab == 0 )
		{
			if ( xui::begin_child( "##world_esp", group_w ) )
			{
				xui::checkbox( "smoke removal", smoke_removal );
				xui::checkbox( "flash removal", flash_removal );
				xui::checkbox( "no scope", no_scope );
				xui::checkbox( "bomb timer", bomb_timer );
				xui::checkbox( "molotov timer", molotov_timer );

				xui::end_child( );
			}

			xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

			if ( xui::begin_child( "##world_esp2", group_w ) )
			{
				xui::checkbox( "dropped weapons", dropped_weapons );
				xui::checkbox( "grenade projectiles", grenade_projectiles );
				xui::checkbox( "grenade tracers", grenade_tracers );
				xui::checkbox( "bullet impacts", impacts );
				xui::checkbox( "spectator list", spectator_list );
				xui::checkbox( "player count", player_count );

				xui::end_child( );
			}
		}
		else if ( this->m_subtab == 1 )
		{
			if ( xui::begin_child( "##world_scene", group_w ) )
			{
				xui::checkbox( "night mode", night_mode );
				xui::checkbox( "no fog", no_fog );
				xui::checkbox( "no sky", no_sky );
				xui::checkbox( "ambient light", ambient_light );

				xui::end_child( );
			}

			xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

			if ( xui::begin_child( "##world_scene2", group_w ) )
			{
				xui::checkbox( "brightness", bright );
				xui::slider_float( "brightness amount", scene_brightness, 0.2f, 3.0f, "%.2f" );

				xui::end_child( );
			}
		}
		else
		{
			if ( xui::begin_child( "##world_weather", group_w ) )
			{
				xui::checkbox( "rain", rain );
				xui::checkbox( "snow", snow );
				xui::checkbox( "stars", stars );
				xui::slider_float( "intensity", weather_intensity, 0.1f, 3.0f, "%.1f" );

				xui::end_child( );
			}
		}
	}

	void menu::draw_skins( float group_w ) const
	{
		using namespace settings::skins;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( this->m_subtab == 0 )
		{
			if ( xui::begin_child( "##skins_guns", group_w ) )
			{
				xui::checkbox( "skins enabled", enabled );
				xui::layout::separator( );

				xui::slider_int( "paint kit", paint_kit, 0, 1200, "%d" );
				xui::slider_int( "seed", seed, 0, 1000, "%d" );
				xui::slider_float( "wear", wear, 0.0f, 1.0f, "%.2f" );
				xui::checkbox( "stattrak", stattrak );
				xui::checkbox( "custom name", custom_name );
				if ( custom_name.value )
				{
					xui::text_input( "name tag##skins", name_tag, 24, "name" );
				}

				xui::end_child( );
			}

			xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

			if ( xui::begin_child( "##skins_guns2", group_w ) )
			{
				xui::text( "weapon specific", tokens::col_text_dim );
				xui::text( "apply paint kit / seed / wear", tokens::col_text_dim );

				xui::end_child( );
			}
		}
		else if ( this->m_subtab == 1 )
		{
			if ( xui::begin_child( "##skins_knives", group_w ) )
			{
				xui::checkbox( "knife enabled", knife_enabled );
				xui::combo( "knife model", knife_model, knife_names, 8 );
				xui::slider_int( "knife paint kit", knife_paint_kit, 0, 1000, "%d" );
				xui::slider_float( "knife wear", wear, 0.0f, 1.0f, "%.2f" );

				xui::end_child( );
			}
		}
		else if ( this->m_subtab == 2 )
		{
			if ( xui::begin_child( "##skins_gloves", group_w ) )
			{
				xui::checkbox( "gloves enabled", gloves_enabled );
				xui::combo( "glove model", gloves_model, glove_names, 6 );
				xui::slider_int( "glove paint kit", gloves_paint_kit, 0, 1000, "%d" );
				xui::slider_float( "glove wear", wear, 0.0f, 1.0f, "%.2f" );

				xui::end_child( );
			}
		}
		else
		{
			if ( xui::begin_child( "##skins_agents", group_w ) )
			{
				xui::checkbox( "agents enabled", agents_enabled );
				xui::slider_int( "agent model", agent_model, 0, 20, "%d" );

				xui::end_child( );
			}
		}
	}

	void menu::draw_misc( float group_w ) const
	{
		using namespace settings::misc;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( this->m_subtab == 0 )
		{
			if ( xui::begin_child( "##misc_main", group_w ) )
			{
				xui::checkbox( "watermark", watermark );
				if ( xui::begin_popup( "##watermark_popup", 220.0f ) )
				{
					xui::checkbox( "show fps##wm", show_fps );
					xui::checkbox( "show time##wm", show_time );
					xui::end_popup( );
				}

				xui::checkbox( "radar hack", radar_hack );
				xui::checkbox( "reveal ranks", reveal_ranks );
				xui::checkbox( "auto peek", auto_peek );

				xui::end_child( );
			}

			xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

			if ( xui::begin_child( "##misc_main2", group_w ) )
			{
				xui::checkbox( "bunny hop", bhop );
				xui::checkbox( "auto strafe", auto_strafe );
				xui::checkbox( "edge jump", edge_jump );
				xui::checkbox( "edge bug", edge_bug );
				xui::checkbox( "fast ladder", fast_ladder );
				xui::checkbox( "slow walk", slow_walk );

				xui::end_child( );
			}
		}
		else if ( this->m_subtab == 1 )
		{
			if ( xui::begin_child( "##misc_removals", group_w ) )
			{
				xui::checkbox( "remove smoke", remove_smoke );
				xui::checkbox( "remove flash", remove_flash );
				xui::checkbox( "remove scope", remove_scope );
				xui::checkbox( "remove postprocess", remove_postprocess );
				xui::checkbox( "remove shadows", remove_shadows );

				xui::end_child( );
			}
		}
		else if ( this->m_subtab == 2 )
		{
			if ( xui::begin_child( "##misc_camera", group_w ) )
			{
				xui::checkbox( "thirdperson", thirdperson );
				xui::checkbox( "camera smoothing", camera_smoothing );
				xui::layout::separator( );
				xui::slider_float( "fov", fov, 80.0f, 140.0f, "%.0f" );
				xui::slider_float( "viewmodel fov", viewmodel_fov, 50.0f, 120.0f, "%.0f" );

				xui::end_child( );
			}
		}
		else
		{
			if ( xui::begin_child( "##misc_hud", group_w ) )
			{
				xui::checkbox( "hitmarker", hitmarker );
				xui::checkbox( "damage indicator", damage_indicator );
				xui::checkbox( "spectators", show_spectators );
				xui::layout::separator( );
				xui::checkbox( "watermark fps##hud", show_fps );
				xui::checkbox( "watermark time##hud", show_time );

				xui::end_child( );
			}
		}
	}

	void menu::draw_config( float group_w )
	{
		using namespace settings::config;

		xui::layout::set_cursor( this->m_body_x - this->m_x, this->m_body_y - this->m_y );

		if ( xui::begin_child( "##config_behaviour", group_w ) )
		{
			xui::checkbox( "load on start", load_on_start );
			xui::checkbox( "save on exit", save_on_exit );

			xui::layout::separator( );

			xui::text( "menu key", tokens::col_text_dim );
			xui::keybind( "menu key##cfg", settings::misc::menu_key );

			xui::layout::separator( );

			if ( xui::button( "unload dll", group_w - 16.0f, 26.0f ) )
			{
				hooks::request_unload( );
			}

			xui::end_child( );
		}

		xui::layout::set_cursor( this->m_body_x - this->m_x + group_w + tokens::gap, this->m_body_y - this->m_y );

		if ( xui::begin_child( "##config_about", group_w ) )
		{
			xui::text( "pastaware v2", tokens::col_accent );
			xui::text( "internal menu for cs2", tokens::col_text_dim );
			xui::layout::separator( );
			xui::text( "insert - toggle menu", tokens::col_text_dim );
			xui::text( "end - unload", tokens::col_text_dim );

			xui::end_child( );
		}
	}

} // namespace rendering
