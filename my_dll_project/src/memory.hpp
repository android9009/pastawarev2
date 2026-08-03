#pragma once

// Small memory / interface helpers used by the internal bootstrap.
// No external dependencies - plain Win32 + stdint.

#include "pch.hpp"

namespace memory {

	/// Read a trivially-copyable value from the given address.
	template<typename T>
	[[nodiscard]] T read( std::uintptr_t address )
	{
		static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable" );
		T out{};
		if ( address )
		{
			std::memcpy( &out, reinterpret_cast< const void* >( address ), sizeof( T ) );
		}
		return out;
	}

	/// Write a trivially-copyable value to the given address.
	template<typename T>
	void write( std::uintptr_t address, const T& value )
	{
		static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable" );
		if ( address )
		{
			std::memcpy( reinterpret_cast< void* >( address ), &value, sizeof( T ) );
		}
	}

	/// Base address of a loaded module (or 0 if not loaded).
	[[nodiscard]] inline std::uintptr_t module_base( const wchar_t* name )
	{
		return reinterpret_cast< std::uintptr_t >( GetModuleHandleW( name ) );
	}

	/// Call a virtual function at `index` on an instance (thiscall on x64 == __fastcall).
	template<typename Ret = void, typename... Args>
	[[nodiscard]] Ret call_vfunc( void* instance, std::size_t index, Args... args )
	{
		const auto vtable = *reinterpret_cast< std::uintptr_t* >( instance );
		const auto fn = reinterpret_cast< Ret( __fastcall* )( void*, Args... ) >( *reinterpret_cast< std::uintptr_t* >( vtable + index * sizeof( std::uintptr_t ) ) );
		return fn( instance, args... );
	}

	/// Read a global interface pointer stored at module_base + rva (cs2-dumper layout).
	[[nodiscard]] inline void* interface_at( std::uintptr_t module_base, std::ptrdiff_t rva )
	{
		if ( !module_base )
		{
			return nullptr;
		}
		return *reinterpret_cast< void** >( module_base + rva );
	}

} // namespace memory
