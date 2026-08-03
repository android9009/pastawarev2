#include "menu.hpp"

#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

namespace menu {
namespace {

constexpr wchar_t k_window_class[] = L"PastAwareV2MenuWindow";
constexpr wchar_t k_window_title[] = L"PastAware";

constexpr int k_width = 500;
constexpr int k_height = 710;
constexpr int k_sidebar_width = 172;
constexpr int k_tab_count = 6;

constexpr UINT k_message_set_visibility = WM_APP + 0x41;
constexpr UINT k_message_shutdown = WM_APP + 0x42;

struct rect {
    int x{};
    int y{};
    int width{};
    int height{};
};

[[nodiscard]] constexpr RECT native_rect(const rect value) {
    return { value.x, value.y, value.x + value.width, value.y + value.height };
}

[[nodiscard]] constexpr bool contains(const rect value, const int x, const int y) {
    return x >= value.x && x < value.x + value.width && y >= value.y && y < value.y + value.height;
}

namespace colors {
constexpr COLORREF background = RGB(13, 15, 20);
constexpr COLORREF sidebar = RGB(17, 20, 27);
constexpr COLORREF surface = RGB(23, 27, 36);
constexpr COLORREF surface_hover = RGB(30, 35, 47);
constexpr COLORREF border = RGB(45, 51, 66);
constexpr COLORREF text = RGB(239, 242, 248);
constexpr COLORREF muted = RGB(142, 150, 169);
constexpr COLORREF accent = RGB(143, 112, 255);
constexpr COLORREF accent_soft = RGB(54, 45, 89);
constexpr COLORREF accent_blue = RGB(98, 205, 255);
constexpr COLORREF success = RGB(98, 219, 166);
constexpr COLORREF switch_off = RGB(62, 69, 84);
} // namespace colors

struct font_bank {
    HFONT display{};
    HFONT heading{};
    HFONT body{};
    HFONT body_bold{};
    HFONT small{};
    HFONT mono{};

    [[nodiscard]] static HFONT make(const int pixel_height, const int weight, const wchar_t* face = L"Segoe UI") {
        return CreateFontW(
            -pixel_height,
            0,
            0,
            0,
            weight,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            face
        );
    }

    void create() {
        display = make(26, FW_SEMIBOLD);
        heading = make(18, FW_SEMIBOLD);
        body = make(14, FW_NORMAL);
        body_bold = make(14, FW_SEMIBOLD);
        small = make(12, FW_NORMAL);
        mono = make(12, FW_SEMIBOLD, L"Cascadia Mono");
    }

    void release() {
        for (const auto font : { display, heading, body, body_bold, small, mono }) {
            if (font) {
                DeleteObject(font);
            }
        }

        display = nullptr;
        heading = nullptr;
        body = nullptr;
        body_bold = nullptr;
        small = nullptr;
        mono = nullptr;
    }
};

struct tab_copy {
    const wchar_t* navigation;
    const wchar_t* title;
    const wchar_t* subtitle;
    const wchar_t* primary_label;
    const wchar_t* primary_description;
    const wchar_t* secondary_label;
    const wchar_t* secondary_description;
    const wchar_t* intensity_label;
};

constexpr std::array<tab_copy, k_tab_count> k_tabs{ {
    { L"Combat", L"Combat", L"Fine-tune the active combat profile.", L"Profile enabled", L"Use this profile when the menu is active.", L"Quiet mode", L"Keep the profile unobtrusive.", L"Profile strength" },
    { L"Visuals", L"Visuals", L"Control the presentation of visual elements.", L"Visual layer", L"Render the selected visual preferences.", L"Clean interface", L"Reduce unnecessary on-screen details.", L"Visual intensity" },
    { L"Movement", L"Movement", L"Configure movement-related preferences.", L"Movement helper", L"Enable the current movement profile.", L"Context aware", L"Apply settings only when appropriate.", L"Assistance level" },
    { L"Inventory", L"Inventory", L"Personalize inventory and cosmetic previews.", L"Inventory preview", L"Show the selected local preview.", L"Remember selection", L"Restore the last chosen preset.", L"Preview scale" },
    { L"Misc", L"Miscellaneous", L"Small quality-of-life preferences live here.", L"Extra tools", L"Enable the selected utility group.", L"Minimal notifications", L"Only show important status messages.", L"Notification level" },
    { L"Settings", L"Settings", L"Appearance, profiles and application behaviour.", L"Remember layout", L"Keep this interface state between sessions.", L"Soft animations", L"Use subtle transitions in the interface.", L"Interface scale" },
} };

struct ui_state {
    int active_tab{};
    int hovered_tab{ -1 };
    bool dragging_slider{};
    std::array<bool, k_tab_count> primary{ true, true, true, true, true, true };
    std::array<bool, k_tab_count> secondary{ true, true, false, true, true, true };
    std::array<float, k_tab_count> intensity{ 0.78f, 0.66f, 0.54f, 0.72f, 0.45f, 0.80f };
    std::array<int, k_tab_count> mode{ 1, 1, 0, 1, 0, 1 };
};

font_bank g_fonts{};
ui_state g_state{};
std::atomic<HWND> g_window{};
std::atomic_bool g_requested_visible{ true };
std::atomic_bool g_visible{};
std::atomic_bool g_stop_requested{};
std::thread g_window_thread{};
std::mutex g_thread_mutex{};

[[nodiscard]] rect tab_rect(const int index) {
    return { 16, 130 + index * 48, 140, 38 };
}

constexpr rect k_close_button{ 448, 20, 28, 28 };
constexpr rect k_primary_toggle{ 424, 143, 38, 22 };
constexpr rect k_secondary_toggle{ 424, 205, 38, 22 };
constexpr rect k_slider_hitbox{ 211, 263, 236, 26 };
constexpr rect k_mode_buttons[3]{ { 212, 385, 74, 30 }, { 297, 385, 74, 30 }, { 382, 385, 74, 30 } };

void fill_rect(HDC dc, const rect value, const COLORREF color) {
    const auto area = native_rect(value);
    const auto brush = CreateSolidBrush(color);
    FillRect(dc, &area, brush);
    DeleteObject(brush);
}

void rounded_rect(HDC dc, const rect value, const int radius, const COLORREF fill, const COLORREF border) {
    const auto brush = CreateSolidBrush(fill);
    const auto pen = CreatePen(PS_SOLID, 1, border);
    const auto old_brush = SelectObject(dc, brush);
    const auto old_pen = SelectObject(dc, pen);

    RoundRect(dc, value.x, value.y, value.x + value.width, value.y + value.height, radius, radius);

    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void line(HDC dc, const int x1, const int y1, const int x2, const int y2, const COLORREF color) {
    const auto pen = CreatePen(PS_SOLID, 1, color);
    const auto old_pen = SelectObject(dc, pen);
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
    SelectObject(dc, old_pen);
    DeleteObject(pen);
}

void circle(HDC dc, const int center_x, const int center_y, const int radius, const COLORREF fill, const COLORREF border) {
    const auto brush = CreateSolidBrush(fill);
    const auto pen = CreatePen(PS_SOLID, 1, border);
    const auto old_brush = SelectObject(dc, brush);
    const auto old_pen = SelectObject(dc, pen);

    Ellipse(dc, center_x - radius, center_y - radius, center_x + radius, center_y + radius);

    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void text(HDC dc, const std::wstring_view value, const rect bounds, const HFONT font, const COLORREF color, const UINT format) {
    const auto old_font = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    auto area = native_rect(bounds);
    DrawTextW(dc, value.data(), static_cast<int>(value.size()), &area, format);
    SelectObject(dc, old_font);
}

[[nodiscard]] std::wstring percent_text(const float value) {
    return std::to_wstring(static_cast<int>(std::lround(value * 100.0f))) + L"%";
}

void draw_switch(HDC dc, const rect bounds, const bool enabled) {
    const auto fill = enabled ? colors::accent : colors::switch_off;
    rounded_rect(dc, bounds, bounds.height, fill, enabled ? colors::accent : colors::border);

    const auto knob_radius = 8;
    const auto knob_x = enabled
        ? bounds.x + bounds.width - bounds.height / 2
        : bounds.x + bounds.height / 2;
    circle(dc, knob_x, bounds.y + bounds.height / 2, knob_radius, RGB(255, 255, 255), RGB(255, 255, 255));
}

void draw_slider(HDC dc, const float value) {
    constexpr rect track{ 212, 274, 234, 4 };
    constexpr int thumb_radius = 7;

    rounded_rect(dc, track, 4, colors::switch_off, colors::switch_off);

    const auto active_width = std::max(4, static_cast<int>(std::lround(track.width * value)));
    rounded_rect(dc, { track.x, track.y, active_width, track.height }, 4, colors::accent, colors::accent);

    const auto thumb_x = track.x + static_cast<int>(std::lround(track.width * value));
    circle(dc, thumb_x, track.y + track.height / 2, thumb_radius, colors::text, colors::accent);
}

void draw_tab(HDC dc, const int index) {
    const auto bounds = tab_rect(index);
    const auto selected = g_state.active_tab == index;
    const auto hovered = g_state.hovered_tab == index;

    if (selected) {
        rounded_rect(dc, bounds, 10, colors::accent_soft, colors::accent_soft);
        fill_rect(dc, { bounds.x, bounds.y + 10, 3, bounds.height - 20 }, colors::accent);
    } else if (hovered) {
        rounded_rect(dc, bounds, 10, colors::surface_hover, colors::surface_hover);
    }

    const auto dot_color = selected ? colors::accent_blue : (hovered ? colors::text : colors::muted);
    circle(dc, bounds.x + 18, bounds.y + bounds.height / 2, 3, dot_color, dot_color);
    text(dc, k_tabs[index].navigation, { bounds.x + 31, bounds.y, 97, bounds.height }, g_fonts.body_bold,
        selected ? colors::text : colors::muted, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void draw_mode_button(HDC dc, const int index, const wchar_t* label) {
    const auto selected = g_state.mode[g_state.active_tab] == index;
    const auto bounds = k_mode_buttons[index];
    rounded_rect(dc, bounds, 8, selected ? colors::accent_soft : colors::background,
        selected ? colors::accent : colors::border);
    text(dc, label, bounds, g_fonts.small, selected ? colors::text : colors::muted,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void draw_menu(HDC dc, const int width, const int height) {
    fill_rect(dc, { 0, 0, width, height }, colors::background);
    fill_rect(dc, { 0, 0, k_sidebar_width, height }, colors::sidebar);
    line(dc, k_sidebar_width, 0, k_sidebar_width, height, colors::border);

    // Brand block.
    rounded_rect(dc, { 18, 22, 34, 34 }, 11, colors::accent_soft, colors::accent_soft);
    circle(dc, 35, 39, 7, colors::accent, colors::accent);
    circle(dc, 35, 39, 3, colors::text, colors::text);
    text(dc, L"PASTAWARE", { 62, 20, 94, 20 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"CS2  /  BUILD 01", { 62, 40, 100, 16 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    text(dc, L"WORKSPACE", { 18, 95, 126, 18 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    for (int index = 0; index < k_tab_count; ++index) {
        draw_tab(dc, index);
    }

    // Sidebar footer.
    line(dc, 18, 600, 154, 600, colors::border);
    circle(dc, 29, 627, 5, colors::success, colors::success);
    text(dc, L"READY", { 42, 616, 90, 18 }, g_fonts.small, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"local session", { 42, 634, 95, 16 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    rounded_rect(dc, { 18, 663, 136, 28 }, 8, colors::surface, colors::border);
    text(dc, L"INSERT  TOGGLE", { 18, 663, 136, 28 }, g_fonts.mono, colors::muted,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    const auto& current = k_tabs[g_state.active_tab];

    // Main header.
    text(dc, current.title, { 196, 25, 220, 34 }, g_fonts.display, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    text(dc, current.subtitle, { 196, 64, 250, 20 }, g_fonts.body, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    rounded_rect(dc, { 390, 66, 86, 23 }, 12, colors::surface, colors::border);
    circle(dc, 404, 77, 3, colors::success, colors::success);
    text(dc, L"LIVE", { 413, 66, 52, 23 }, g_fonts.small, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    rounded_rect(dc, k_close_button, 8, colors::surface, colors::border);
    text(dc, L"\x00D7", k_close_button, g_fonts.heading, colors::muted,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // General card.
    rounded_rect(dc, { 196, 120, 280, 170 }, 14, colors::surface, colors::border);
    text(dc, L"GENERAL", { 212, 134, 130, 18 }, g_fonts.small, colors::accent_blue,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    text(dc, current.primary_label, { 212, 154, 190, 20 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    text(dc, current.primary_description, { 212, 174, 190, 17 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    draw_switch(dc, k_primary_toggle, g_state.primary[g_state.active_tab]);

    line(dc, 212, 196, 460, 196, colors::border);
    text(dc, current.secondary_label, { 212, 207, 190, 20 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    text(dc, current.secondary_description, { 212, 227, 190, 17 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    draw_switch(dc, k_secondary_toggle, g_state.secondary[g_state.active_tab]);

    line(dc, 212, 249, 460, 249, colors::border);
    text(dc, current.intensity_label, { 212, 254, 150, 20 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    text(dc, percent_text(g_state.intensity[g_state.active_tab]), { 400, 254, 47, 20 }, g_fonts.small, colors::accent_blue,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    draw_slider(dc, g_state.intensity[g_state.active_tab]);

    // Profile card.
    rounded_rect(dc, { 196, 310, 280, 156 }, 14, colors::surface, colors::border);
    text(dc, L"PROFILE", { 212, 325, 130, 18 }, g_fonts.small, colors::accent_blue,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"Preset behaviour", { 212, 347, 190, 21 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"Choose the balance that fits this page.", { 212, 367, 236, 17 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    draw_mode_button(dc, 0, L"Soft");
    draw_mode_button(dc, 1, L"Balanced");
    draw_mode_button(dc, 2, L"Focused");

    rounded_rect(dc, { 212, 431, 248, 20 }, 7, colors::background, colors::background);
    text(dc, L"Click a preset to switch instantly", { 221, 431, 220, 20 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Hotkey card.
    rounded_rect(dc, { 196, 486, 280, 102 }, 14, colors::surface, colors::border);
    text(dc, L"QUICK ACCESS", { 212, 501, 145, 18 }, g_fonts.small, colors::accent_blue,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"Menu hotkey", { 212, 523, 130, 20 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"Open or hide the interface at any time.", { 212, 545, 176, 17 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    rounded_rect(dc, { 389, 523, 71, 30 }, 8, colors::background, colors::border);
    text(dc, L"INSERT", { 389, 523, 71, 30 }, g_fonts.mono, colors::text,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Status card.
    rounded_rect(dc, { 196, 608, 280, 74 }, 14, colors::surface, colors::border);
    circle(dc, 221, 634, 5, colors::success, colors::success);
    text(dc, L"Interface ready", { 235, 621, 150, 19 }, g_fonts.body_bold, colors::text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    text(dc, L"All controls are local and responsive.", { 235, 641, 205, 17 }, g_fonts.small, colors::muted,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

[[nodiscard]] int hit_tab(const int x, const int y) {
    for (int index = 0; index < k_tab_count; ++index) {
        if (contains(tab_rect(index), x, y)) {
            return index;
        }
    }

    return -1;
}

void set_slider_value(const int x) {
    constexpr int track_x = 212;
    constexpr int track_width = 234;
    const auto value = static_cast<float>(x - track_x) / static_cast<float>(track_width);
    g_state.intensity[g_state.active_tab] = std::clamp(value, 0.0f, 1.0f);
}

void set_visibility(const HWND window, const bool visible) {
    g_requested_visible.store(visible, std::memory_order_release);
    g_visible.store(visible, std::memory_order_release);

    if (visible) {
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
        ShowWindow(window, SW_SHOW);
        SetForegroundWindow(window);
    } else {
        ShowWindow(window, SW_HIDE);
    }
}

struct host_window_search {
    DWORD process_id{};
    HWND window{};
    std::int64_t largest_area{};
};

BOOL CALLBACK find_host_window(const HWND window, const LPARAM parameter) {
    auto& search = *reinterpret_cast<host_window_search*>(parameter);

    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    if (process_id != search.process_id || !IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr) {
        return TRUE;
    }

    RECT bounds{};
    if (!GetWindowRect(window, &bounds)) {
        return TRUE;
    }

    const auto width = static_cast<std::int64_t>(bounds.right - bounds.left);
    const auto height = static_cast<std::int64_t>(bounds.bottom - bounds.top);
    const auto area = width * height;
    if (width > 0 && height > 0 && area > search.largest_area) {
        search.window = window;
        search.largest_area = area;
    }

    return TRUE;
}

[[nodiscard]] POINT initial_position() {
    RECT work_area{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);

    host_window_search search{};
    search.process_id = GetCurrentProcessId();
    EnumWindows(find_host_window, reinterpret_cast<LPARAM>(&search));

    RECT target = work_area;
    if (search.window) {
        RECT host_bounds{};
        if (GetWindowRect(search.window, &host_bounds)) {
            target = host_bounds;
        }
    }

    const auto min_x = work_area.left;
    const auto max_x = std::max(min_x, work_area.right - k_width);
    const auto min_y = work_area.top;
    const auto max_y = std::max(min_y, work_area.bottom - k_height);

    return {
        std::clamp(target.left + ((target.right - target.left) - k_width) / 2, min_x, max_x),
        std::clamp(target.top + ((target.bottom - target.top) - k_height) / 2, min_y, max_y),
    };
}

void invalidate(const HWND window) {
    InvalidateRect(window, nullptr, FALSE);
}

LRESULT CALLBACK window_proc(const HWND window, const UINT message, const WPARAM w_param, const LPARAM l_param) {
    switch (message) {
    case WM_CREATE: {
        // The popup has no standard frame, so give the whole surface soft corners.
        const auto region = CreateRoundRectRgn(0, 0, k_width + 1, k_height + 1, 18, 18);
        if (region && !SetWindowRgn(window, region, TRUE)) {
            DeleteObject(region);
        }

        g_fonts.create();
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        const auto paint_dc = BeginPaint(window, &paint);

        RECT client{};
        GetClientRect(window, &client);
        const auto width = client.right - client.left;
        const auto height = client.bottom - client.top;

        const auto buffer_dc = CreateCompatibleDC(paint_dc);
        const auto bitmap = CreateCompatibleBitmap(paint_dc, width, height);
        const auto old_bitmap = SelectObject(buffer_dc, bitmap);

        draw_menu(buffer_dc, width, height);
        BitBlt(paint_dc, 0, 0, width, height, buffer_dc, 0, 0, SRCCOPY);

        SelectObject(buffer_dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(buffer_dc);
        EndPaint(window, &paint);
        return 0;
    }

    case WM_NCHITTEST: {
        const auto x = GET_X_LPARAM(l_param);
        const auto y = GET_Y_LPARAM(l_param);
        POINT point{ x, y };
        ScreenToClient(window, &point);

        if (point.y >= 0 && point.y < 82 && !contains(k_close_button, point.x, point.y)) {
            return HTCAPTION;
        }
        return HTCLIENT;
    }

    case WM_MOUSEMOVE: {
        const auto hovered = hit_tab(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
        if (hovered != g_state.hovered_tab) {
            g_state.hovered_tab = hovered;
            invalidate(window);
        }

        if (g_state.dragging_slider) {
            set_slider_value(GET_X_LPARAM(l_param));
            invalidate(window);
        }

        TRACKMOUSEEVENT tracking{ sizeof(tracking), TME_LEAVE, window, 0 };
        TrackMouseEvent(&tracking);
        return 0;
    }

    case WM_MOUSELEAVE:
        if (g_state.hovered_tab != -1) {
            g_state.hovered_tab = -1;
            invalidate(window);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        const auto x = GET_X_LPARAM(l_param);
        const auto y = GET_Y_LPARAM(l_param);

        if (contains(k_close_button, x, y)) {
            set_visibility(window, false);
            return 0;
        }

        if (const auto tab = hit_tab(x, y); tab != -1) {
            g_state.active_tab = tab;
            invalidate(window);
            return 0;
        }

        if (contains(k_primary_toggle, x, y)) {
            g_state.primary[g_state.active_tab] = !g_state.primary[g_state.active_tab];
            invalidate(window);
            return 0;
        }

        if (contains(k_secondary_toggle, x, y)) {
            g_state.secondary[g_state.active_tab] = !g_state.secondary[g_state.active_tab];
            invalidate(window);
            return 0;
        }

        if (contains(k_slider_hitbox, x, y)) {
            g_state.dragging_slider = true;
            SetCapture(window);
            set_slider_value(x);
            invalidate(window);
            return 0;
        }

        for (int index = 0; index < 3; ++index) {
            if (contains(k_mode_buttons[index], x, y)) {
                g_state.mode[g_state.active_tab] = index;
                invalidate(window);
                return 0;
            }
        }

        return 0;
    }

    case WM_LBUTTONUP:
        if (g_state.dragging_slider) {
            g_state.dragging_slider = false;
            ReleaseCapture();
            invalidate(window);
        }
        return 0;

    case WM_KEYDOWN:
        if (w_param == VK_ESCAPE) {
            set_visibility(window, false);
            return 0;
        }
        return 0;

    case k_message_set_visibility:
        set_visibility(window, w_param != FALSE);
        return 0;

    case k_message_shutdown:
        DestroyWindow(window);
        return 0;

    case WM_CLOSE:
        set_visibility(window, false);
        return 0;

    case WM_DESTROY:
        g_visible.store(false, std::memory_order_release);
        g_window.store(nullptr, std::memory_order_release);
        g_fonts.release();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(window, message, w_param, l_param);
    }
}

void window_thread() {
    const auto instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = k_window_class;

    const auto class_atom = RegisterClassExW(&window_class);
    if (!class_atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return;
    }

    const auto position = initial_position();
    const auto window = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        k_window_class,
        k_window_title,
        WS_POPUP,
        position.x,
        position.y,
        k_width,
        k_height,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!window) {
        if (class_atom) {
            UnregisterClassW(k_window_class, instance);
        }
        return;
    }

    g_window.store(window, std::memory_order_release);

    if (g_stop_requested.load(std::memory_order_acquire)) {
        PostMessageW(window, k_message_shutdown, 0, 0);
    } else {
        set_visibility(window, g_requested_visible.load(std::memory_order_acquire));
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (class_atom) {
        UnregisterClassW(k_window_class, instance);
    }
}

} // namespace

bool start() {
    std::lock_guard lock(g_thread_mutex);
    if (g_window_thread.joinable()) {
        return true;
    }

    g_stop_requested.store(false, std::memory_order_release);
    g_requested_visible.store(true, std::memory_order_release);

    try {
        g_window_thread = std::thread(window_thread);
        return true;
    } catch (...) {
        return false;
    }
}

void stop() {
    std::thread worker;
    {
        std::lock_guard lock(g_thread_mutex);
        if (!g_window_thread.joinable()) {
            return;
        }

        g_stop_requested.store(true, std::memory_order_release);
        g_requested_visible.store(false, std::memory_order_release);

        if (const auto window = g_window.load(std::memory_order_acquire)) {
            PostMessageW(window, k_message_shutdown, 0, 0);
        }

        worker = std::move(g_window_thread);
    }

    if (worker.get_id() == std::this_thread::get_id()) {
        worker.detach();
    } else {
        worker.join();
    }
}

void toggle() {
    const auto visible = !g_requested_visible.load(std::memory_order_acquire);
    g_requested_visible.store(visible, std::memory_order_release);

    if (const auto window = g_window.load(std::memory_order_acquire)) {
        PostMessageW(window, k_message_set_visibility, visible ? TRUE : FALSE, 0);
    }
}

bool is_visible() {
    return g_visible.load(std::memory_order_acquire);
}

} // namespace menu
