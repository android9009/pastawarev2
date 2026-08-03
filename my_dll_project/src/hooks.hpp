#pragma once

// Internal bootstrap: hooks the game's D3D11 swap chain (Present /
// ResizeBuffers) and window procedure so the menu can render in-game.
// No external hooking library - plain vtable patch + window subclass.

#include "pch.hpp"

namespace hooks {

	/// Finds the CS2 window, grabs the DXGI swap-chain vtable via a dummy
	/// swap chain and patches Present/ResizeBuffers. Idempotent.
	bool initialize( );

	/// Restores the vtable and window procedure, releases the mouse lock.
	void shutdown( );

	[[nodiscard]] bool is_ready( );
	[[nodiscard]] HWND window( );

	/// Asks the main thread to unload the DLL (checked by dllmain).
	void request_unload( );
	[[nodiscard]] bool unload_requested( );

	// ── CInputSystem mouse lock (menu needs a free cursor) ──────────────────
	void set_relative_mouse( bool relative );
	[[nodiscard]] std::uint8_t relative_mouse( );

} // namespace hooks
