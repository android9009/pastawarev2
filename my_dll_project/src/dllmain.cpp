#include "pch.hpp"

#include "hooks.hpp"
#include "menu.hpp"
#include "context.hpp"

// Основная логика, работающая в отдельном потоке после загрузки DLL.
// Никакой консоли и внешних окон - всё рисуется прямо в игре.
DWORD WINAPI MainThread( LPVOID lpParam )
{
	// Ждём появления окна игры и цепляем хуки (идемпотентно, повторяем).
	// Клавиша меню обрабатывается в перехваченном WndProc (по умолчанию
	// INSERT), здесь остаётся только контроль выгрузки по END.
	while ( !hooks::unload_requested( ) )
	{
		if ( !hooks::is_ready( ) )
		{
			hooks::initialize( );
		}

		if ( GetAsyncKeyState( VK_END ) & 1 )
		{
			hooks::request_unload( );
			break;
		}

		Sleep( 10 );
	}

	// Корректно снимаем хуки и освобождаем лок мыши, если он был взят.
	rendering::g_menu.shutdown( );
	rendering::g_context.shutdown( );
	hooks::shutdown( );

	FreeLibraryAndExitThread( static_cast< HMODULE >( lpParam ), 0 );
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	switch ( ul_reason_for_call )
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls( hModule );
		CreateThread( nullptr, 0, MainThread, hModule, 0, nullptr );
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
