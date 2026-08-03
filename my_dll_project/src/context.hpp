#pragma once

// D3D11 render context: owns the device/context/RTV for the game's swap
// chain and drives the frame render (watermark, keybinds, menu).

#include "pch.hpp"

namespace rendering {

	class context
	{
	public:
		bool initialize( IDXGISwapChain* swap_chain );
		void shutdown( );

		void on_present( IDXGISwapChain* swap_chain );
		void on_resize_buffers( );
		void on_resize_buffers_post( IDXGISwapChain* swap_chain );

		[[nodiscard]] ID3D11Device* get_device( ) const { return this->m_device; }
		[[nodiscard]] ID3D11DeviceContext* get_context( ) const { return this->m_context; }
		[[nodiscard]] HWND get_window( ) const { return this->m_window; }
		[[nodiscard]] bool is_initialized( ) const { return this->m_initialized; }

	private:
		void create_rtv( IDXGISwapChain* swap_chain );

		ID3D11Device* m_device{ nullptr };
		ID3D11DeviceContext* m_context{ nullptr };
		ID3D11RenderTargetView* m_rtv{ nullptr };
		HWND m_window{ nullptr };
		bool m_initialized{ false };
		bool m_ui_assets_ready{ false };
	};

	inline context g_context{ };

} // namespace rendering
