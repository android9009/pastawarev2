#pragma once

namespace menu {

// Starts the native UI on its own thread. The menu is shown after creation.
[[nodiscard]] bool start();

// Stops the UI thread and releases all Win32/GDI resources.
void stop();

// Toggles the menu without doing UI work on the caller's thread.
void toggle();

[[nodiscard]] bool is_visible();

} // namespace menu
