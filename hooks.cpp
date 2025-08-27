#include "includes.h"

#include "dependencies/include/minhook/MinHook.h"
#include <intrin.h>
#include <stdexcept>

#include "dependencies/include/imgui/imgui.h"
#include "dependencies/include/imgui/imgui_impl_win32.h"
#include "dependencies/include/imgui/imgui_impl_dx9.h"

Hooks g_hooks{ };

void Hooks::init( ) {
	if ( MH_Initialize( ) )
		throw std::runtime_error( "unable to init minhook" );

	if ( MH_CreateHook( g_memory.Get( g_gui.device, 42 ), &EndScene, reinterpret_cast< void** >( &EndSceneOriginal ) ) )
		throw std::runtime_error( "Unable to hook EndScene()" );

	if ( MH_CreateHook( g_memory.Get( g_gui.device, 16 ), &Reset, reinterpret_cast< void** >( &ResetOriginal ) ) )
		throw std::runtime_error( "Unable to hook Reset()" );

	// AllocKeyValuesMemory hook
	/*
	if ( MH_CreateHook( g_memory.Get( interfaces::keyValuesSystem, 1 ), &AllocKeyValuesMemory, reinterpret_cast< void** >( &AllocKeyValuesMemoryOriginal ) ) )
		throw std::runtime_error( "Unable to hook AllocKeyValuesMemory()" );

	// CreateMove hook
	if ( MH_CreateHook( g_memory.Get( interfaces::clientMode, 22 ), &CreateMove, reinterpret_cast< void** >( &CreateMoveOriginal ) ) )
		throw std::runtime_error( "Unable to hook CreateMove()" );
	*/

	g_gui.DestroyDirectX( );
	MH_EnableHook( MH_ALL_HOOKS );


}

void Hooks::destroy( ) {
	MH_DisableHook( MH_ALL_HOOKS );
	MH_RemoveHook( MH_ALL_HOOKS );

	MH_Uninitialize( );
}

long __stdcall Hooks::EndScene( IDirect3DDevice9* device ) noexcept {
	static const auto returnAddress = _ReturnAddress( );

	const auto result = EndSceneOriginal( device, device );

	// stop EndScene getting called twice
	if ( _ReturnAddress( ) == returnAddress )
		return result;

	if ( !g_gui.setup )
		g_gui.SetupMenu( device );

	if ( g_gui.open )
		g_gui.Render( );

	return result;
}

HRESULT __stdcall Hooks::Reset( IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params ) noexcept {
	ImGui_ImplDX9_InvalidateDeviceObjects( );
	const auto result = ResetOriginal( device, device, params );
	ImGui_ImplDX9_CreateDeviceObjects( );
	return result;
}