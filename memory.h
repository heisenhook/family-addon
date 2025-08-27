#pragma once
#include <cstdint>
#include <functional>

class Memory {
public:
	void init( );

	// call virtual function @ given index
	template <typename Return, typename ... Arguments>
	constexpr Return Call( void* vmt, const std::uint32_t index, Arguments ... args ) noexcept
	{
		using Function = Return( __thiscall* )( void*, decltype( args )... );
		return ( *static_cast< Function** >( vmt ) )[ index ]( vmt, args... );
	}

	// get void pointer to virtual function @ given index
	constexpr void* Get( void* vmt, const std::uint32_t index ) noexcept
	{
		return ( *static_cast< void*** >( vmt ) )[ index ];
	}

	inline std::uintptr_t RelativeToAbsolute( std::uintptr_t relAddr ) noexcept {
		return static_cast< std::uintptr_t >( relAddr + 4 + *reinterpret_cast< std::int32_t* >( relAddr ) );
	}

	// simple Pattern/AOB/Signature scanner
	std::uint8_t* PatternScan( const char* moduleName, const char* pattern ) noexcept;

	// return addresses
	//std::uint8_t* allocKeyValuesClient = nullptr;
	//std::uint8_t* allocKeyValuesEngine = nullptr;
	//std::uint8_t* insertIntoTree = nullptr;

	// functions
	//std::uint8_t* keyValuesFromString = nullptr;

	// interfaces
	//std::uint8_t* glowManager = nullptr;

};

extern Memory g_memory;