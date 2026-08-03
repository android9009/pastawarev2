#pragma once

// ── common macros ────────────────────────────────────────────────────────
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// ── windows ──────────────────────────────────────────────────────────────
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3d11_1.h>

// ── standard library ─────────────────────────────────────────────────────
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// rpcndr.h may export this legacy macro; it is unsafe in modern C++ code.
#ifdef small
#undef small
#endif
