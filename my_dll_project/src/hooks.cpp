#include "pch.hpp"

#include "hooks.hpp"
#include "memory.hpp"
#include "menu.hpp"
#include "settings.hpp"
#include "context.hpp"
#include "../../offset-pattern/interfaces.hpp"
#include "../external/xdraw/xui/xui.hpp"

namespace hooks {

	namespace {

		HWND g_window{ nullptr };
		WNDPROC g_o_wndproc{ nullptr };

		std::uintptr_t* g_vtable{ nullptr };
		std::uintptr_t g_o_present{ 0 };
		std::uintptr_t g_o_resize{ 0 };

		void* g_input_system{ nullptr };

		std::atomic<bool> g_unload_requested{ false };
		std::atomic<bool> g_ready{ false };

		// ── present / resize stubs ────────────────────────────────────────────

		HRESULT __stdcall present_hook( IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags )
		{
			rendering::g_context.on_present( swap_chain );

			return reinterpret_cast< HRESULT( __stdcall* )( IDXGISwapChain*, UINT, UINT ) >( g_o_present )( swap_chain, sync_interval, flags );
		}

		HRESULT __stdcall resize_hook( IDXGISwapChain* swap_chain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swap_chain_flags )
		{
			rendering::g_context.on_resize_buffers( );

			const auto result = reinterpret_cast< HRESULT( __stdcall* )( IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT ) >( g_o_resize )( swap_chain, buffer_count, width, height, new_format, swap_chain_flags );

			if ( SUCCEEDED( result ) )
			{
				rendering::g_context.on_resize_buffers_post( swap_chain );
			}

			return result;
		}

		// ── window procedure ─────────────────────────────────────────────────

		LRESULT __stdcall wnd_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
		{
			if ( msg == WM_ACTIVATE && LOWORD( wparam ) != WA_INACTIVE && rendering::g_menu.is_open( ) && rendering::g_context.get_window( ) == hwnd )
			{
				set_relative_mouse( false );
				SetCursor( LoadCursor( nullptr, IDC_ARROW ) );
				rendering::g_menu.apply_saved_cursor( );
			}

			if ( msg == WM_KEYDOWN && !( lparam & ( 1 << 30 ) ) && static_cast< int >( wparam ) == settings::misc::menu_key )
			{
				rendering::g_menu.toggle( );
				return 0;
			}

			xui::wndproc( msg, wparam, lparam );

			if ( rendering::g_menu.is_open( ) )
			{
				// while the menu is open, keep mouse input away from the game
				switch ( msg )
				{
				case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
				case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
				case WM_MOUSEMOVE:
					return 0;
				default:
					break;
				}
			}

			return CallWindowProcW( g_o_wndproc, hwnd, msg, wparam, lparam );
		}

		// ── swap chain vtable acquisition ────────────────────────────────────
		// Create a throwaway D3D11 device + swap chain on a hidden window and
		// copy the vtable - every swap chain in the process shares it.

		bool acquire_vtable( )
		{
			WNDCLASSEXW wc{ };
			wc.cbSize = sizeof( wc );
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = DefWindowProcW;
			wc.hInstance = GetModuleHandleW( nullptr );
			wc.lpszClassName = L"PastAwareDummyWindow";
			RegisterClassExW( &wc );

			const auto dummy = CreateWindowExW( 0, L"PastAwareDummyWindow", L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr );
			if ( !dummy )
			{
				return false;
			}

			DXGI_SWAP_CHAIN_DESC desc{ };
			desc.BufferDesc.Width = 1;
			desc.BufferDesc.Height = 1;
			desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BufferDesc.RefreshRate = { 60, 1 };
			desc.SampleDesc.Count = 1;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = 1;
			desc.OutputWindow = dummy;
			desc.Windowed = TRUE;
			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			constexpr D3D_FEATURE_LEVEL levels[ ]
			{
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
			};

			IDXGISwapChain* swap_chain{ nullptr };
			ID3D11Device* device{ nullptr };
			ID3D11DeviceContext* context{ nullptr };

			const auto hr = D3D11CreateDeviceAndSwapChain(
				nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
				levels, static_cast< UINT >( std::size( levels ) ), D3D11_SDK_VERSION,
				&desc, &swap_chain, &device, nullptr, &context );

			if ( FAILED( hr ) || !swap_chain )
			{
				if ( swap_chain ) swap_chain->Release( );
				if ( context ) context->Release( );
				if ( device ) device->Release( );
				DestroyWindow( dummy );
				return false;
			}

			g_vtable = *reinterpret_cast< std::uintptr_t** >( swap_chain );

			swap_chain->Release( );
			if ( context ) context->Release( );
			if ( device ) device->Release( );
			DestroyWindow( dummy );
			return true;
		}

		void patch_vtable( )
		{
			MEMORY_BASIC_INFORMATION mbi{ };
			VirtualQuery( g_vtable, &mbi, sizeof( mbi ) );

			DWORD old_protect{ 0 };
			VirtualProtect( mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &old_protect );

			// IDXGISwapChain vtable: Present = 8, ResizeBuffers = 13
			g_o_present = g_vtable[ 8 ];
			g_o_resize = g_vtable[ 13 ];

			g_vtable[ 8 ] = reinterpret_cast< std::uintptr_t >( &present_hook );
			g_vtable[ 13 ] = reinterpret_cast< std::uintptr_t >( &resize_hook );

			VirtualProtect( mbi.BaseAddress, mbi.RegionSize, old_protect, &old_protect );
		}

		void unpatch_vtable( )
		{
			if ( !g_vtable || !g_o_present || !g_o_resize )
			{
				return;
			}

			MEMORY_BASIC_INFORMATION mbi{ };
			VirtualQuery( g_vtable, &mbi, sizeof( mbi ) );

			DWORD old_protect{ 0 };
			VirtualProtect( mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &old_protect );

			g_vtable[ 8 ] = g_o_present;
			g_vtable[ 13 ] = g_o_resize;

			VirtualProtect( mbi.BaseAddress, mbi.RegionSize, old_protect, &old_protect );
		}

		void* find_input_system( )
		{
			const auto base = memory::module_base( L"inputsystem.dll" );
			if ( !base )
			{
				return nullptr;
			}

			return memory::interface_at( base, cs2_dumper::interfaces::inputsystem_dll::InputSystemVersion001 );
		}

	} // namespace

	bool initialize( )
	{
		if ( g_ready.load( std::memory_order_acquire ) )
		{
			return true;
		}

		if ( !g_window )
		{
			// CS2's window class; fall back to the foreground window so the
			// hook can be smoke-tested in other D3D11 apps too.
			g_window = FindWindowW( L"SDL_app", nullptr );
			if ( !g_window )
			{
				g_window = GetForegroundWindow( );
			}
			if ( !g_window )
			{
				return false;
			}
		}

		if ( !acquire_vtable( ) )
		{
			return false;
		}

		patch_vtable( );

		g_o_wndproc = reinterpret_cast< WNDPROC >( SetWindowLongPtrW( g_window, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( &wnd_proc ) ) );

		g_input_system = find_input_system( );

		g_ready.store( true, std::memory_order_release );
		return true;
	}

	void shutdown( )
	{
		if ( !g_ready.load( std::memory_order_acquire ) )
		{
			return;
		}

		set_relative_mouse( true );

		if ( g_window && g_o_wndproc )
		{
			SetWindowLongPtrW( g_window, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( g_o_wndproc ) );
			g_o_wndproc = nullptr;
		}

		unpatch_vtable( );

		g_ready.store( false, std::memory_order_release );
	}

	bool is_ready( )
	{
		return g_ready.load( std::memory_order_acquire );
	}

	HWND window( )
	{
		return g_window;
	}

	void request_unload( )
	{
		g_unload_requested.store( true, std::memory_order_release );
	}

	bool unload_requested( )
	{
		return g_unload_requested.load( std::memory_order_acquire );
	}

	void set_relative_mouse( bool relative )
	{
		if ( !g_input_system )
		{
			return;
		}

		// CInputSystem::SetRelativeMouseMode - vtable index 76 (velocity).
		memory::call_vfunc<void>( g_input_system, 76, relative );
	}

	std::uint8_t relative_mouse( )
	{
		if ( !g_input_system )
		{
			return 0;
		}

		// m_bRelativeMouseMode, offset 0x54 (velocity).
		return memory::read<std::uint8_t>( reinterpret_cast< std::uintptr_t >( g_input_system ) + 0x54 );
	}

} // namespace hooks
