#include "includes.h"
#include "address.h"

#include "MinHook.h"
#include <intrin.h>
#include <stdexcept>
#include <unordered_set>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"

Hooks g_hooks{ };
std::unordered_set<Checkbox*> g_checkboxInstances;
std::unordered_set<Dropdown*> g_dropdownInstances;
std::unordered_set<MultiDropdown*> g_multiDropdownInstances;
std::unordered_set<Slider*> g_sliderInstances;

struct WatcherParams {
    uintptr_t idaFunctionVA;
    uintptr_t idaImageBase;
    const wchar_t* moduleName;
    unsigned char* verifyBytes;
    size_t verifyLen;
    unsigned char* pattern;
    const char* mask;
};

static DWORD WINAPI ModuleWatcherThreadProc( LPVOID lpParam ) {
    WatcherParams* p = reinterpret_cast< WatcherParams* >( lpParam );
    if ( !p ) return 1;

    const wchar_t* moduleName = p->moduleName;
    DWORD pollMs = 200;
    const DWORD timeoutMs = 30000; // 30s timeout; change to INFINITE if you prefer
    DWORD startTick = GetTickCount( );

    // debug print
    {
        char buf[ 256 ];
        sprintf_s( buf, "ModuleWatcher: waiting for %ls\n", L"HisHolySpiritOurLordAndSaviorJesusChrist.dll" );
        OutputDebugStringA( buf );
    }

    HMODULE hMod = nullptr;
    while ( true ) {
        hMod = GetModuleHandleW( L"HisHolySpiritOurLordAndSaviorJesusChrist.dll" );
        if ( hMod ) break;

        if ( ( GetTickCount( ) - startTick ) > timeoutMs ) {
            OutputDebugStringA( "ModuleWatcher: timeout waiting for module\n" );
            break;
        }
        Sleep( pollMs );
    }

    if ( !hMod ) {
        delete p;
        return 1;
    }

    // compute runtime address from IDA VA and IDA image base
    uintptr_t rva = p->idaFunctionVA - p->idaImageBase;
    uintptr_t runtimeAddr = reinterpret_cast< uintptr_t >( hMod ) + rva;

    // verify stable bytes first
    bool ok = VerifyBytes( runtimeAddr, p->verifyBytes, p->verifyLen );
    if ( !ok ) {
        // attempt pattern scan fallback
        uintptr_t found = PatternScanModule( p->moduleName, p->pattern, p->mask );
        if ( found ) {
            runtimeAddr = found;
        }
        else {
            char buf[ 256 ];
            sprintf_s( buf, "ModuleWatcher: failed to find Checkbox ctor; runtimeAddr=0x%p\n", ( void* )runtimeAddr );
            OutputDebugStringA( buf );
            delete p;
            return 1;
        }
    }

    {
        char buf[ 256 ];
        sprintf_s( buf, "ModuleWatcher: resolved Checkbox ctor at 0x%p\n", ( void* )runtimeAddr );
        OutputDebugStringA( buf );
    }

    // Create the hook for Checkbox ctor
    /*
    if ( MH_CreateHook( reinterpret_cast< LPVOID >( runtimeAddr ),
        &Hooks::CheckboxCtorHook,
        reinterpret_cast< void** >( &Hooks::OriginalCheckboxCtor ) ) != MH_OK ) {
        char buf[ 256 ];
        sprintf_s( buf, "ModuleWatcher: MH_CreateHook failed for Checkbox ctor at 0x%p\n", ( void* )runtimeAddr );
        OutputDebugStringA( buf );
        delete p;
        return 1;
    }
    */

    // enable hooks now (enable all hooks)
    if ( MH_EnableHook( MH_ALL_HOOKS ) != MH_OK ) {
        OutputDebugStringA( "ModuleWatcher: MH_EnableHook failed\n" );
        // still continue; caller may have other fallback
    }
    else {
        OutputDebugStringA( "ModuleWatcher: Checkbox ctor hooked and hooks enabled\n" );
    }

    delete p;
    return 0;
}

void Hooks::init( ) {
	if ( MH_Initialize( ) )
		throw std::runtime_error( "unable to init minhook" );

	if ( MH_CreateHook( g_memory.Get( g_gui.device, 42 ), &EndScene, reinterpret_cast< void** >( &EndSceneOriginal ) ) )
		throw std::runtime_error( "Unable to hook EndScene()" );

	if ( MH_CreateHook( g_memory.Get( g_gui.device, 16 ), &Reset, reinterpret_cast< void** >( &ResetOriginal ) ) )
		throw std::runtime_error( "Unable to hook Reset()" );

    if (MH_CreateHook(g_memory.PatternScan("HisHolySpiritOurLordAndSaviorJesusChrist.dll", "55 8B EC 83 EC ? 56 8B F1 80 BE 8D 00 00 00 ?"), &Checkbox__Think, reinterpret_cast<void**>(&oCheckbox__Think)))
        throw std::runtime_error("Unable to hook Checkbox::Think()");
    
    if ( MH_CreateHook( g_memory.PatternScan( "HisHolySpiritOurLordAndSaviorJesusChrist.dll", "55 8B EC 83 EC ? 56 8B F1 57 8B 86 F0 00 00 00 8B 8E F4 00 00 00 3B C1 0F 84 ? ? ? ? 2B C8 B8 ? ? ? ? F7 E9 C5 F8 57 C0 8B 0D ? ? ? ? C1 FA ? 8B FA C1 EF ? 8B 01 03 FA C1 E7 ? C5 FA 2A C7 C5 FA 5E 05 ? ? ? ? 8B 40 30 89 45 FC C5 FA 11 45 F8 FF 55 FC D9 ? ? C5 FA 10 45 FC C5 E8 57 D2 C5 F8 2F C2 76 ? 8B 0D ? ? ? ? 8B 01 8B 40 30 89 45 FC FF 55 FC D9 ? ? C5 FA 10 4D FC C5 E8 57 D2 EB ? C5 FA 10 0D ? ? ? ? A1 ? ? ? ? 8D 8E FC 00 00 00" ), &Dropdown__Think, reinterpret_cast< void** >( &oDropdown__Think ) ) )
        throw std::runtime_error( "Unable to hook Dropdown::Think()" );

    if ( MH_CreateHook( g_memory.PatternScan( "HisHolySpiritOurLordAndSaviorJesusChrist.dll", "55 8B EC 83 EC ? 56 8B F1 57 8B 86 F0 00 00 00 8B 8E F4 00 00 00 3B C1 0F 84 ? ? ? ? 2B C8 B8 ? ? ? ? F7 E9 C5 F8 57 C0 8B 0D ? ? ? ? C1 FA ? 8B FA C1 EF ? 8B 01 03 FA C1 E7 ? C5 FA 2A C7 C5 FA 5E 05 ? ? ? ? 8B 40 30 89 45 FC C5 FA 11 45 F8 FF 55 FC D9 ? ? C5 FA 10 45 FC C5 E8 57 D2 C5 F8 2F C2 76 ? 8B 0D ? ? ? ? 8B 01 8B 40 30 89 45 FC FF 55 FC D9 ? ? C5 FA 10 4D FC C5 E8 57 D2 EB ? C5 FA 10 0D ? ? ? ? A1 ? ? ? ? 8D 8E 0C 01 00 00" ), &MultiDropdown__Think, reinterpret_cast< void** >( &oMultiDropdown__Think ) ) )
        throw std::runtime_error( "Unable to hook MultiDropdown::Think()" );

    if ( MH_CreateHook( g_memory.PatternScan( "HisHolySpiritOurLordAndSaviorJesusChrist.dll", "55 8B EC 83 EC ? 53 56 8B F1 C5 F0 57 C9" ), &Slider__Think, reinterpret_cast< void** >( &oSlider__Think ) ) )
        throw std::runtime_error( "Unable to hook Slider::Think()" );


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
	// If watcher thread still running, wait briefly and close handle
	if ( g_hooks.watcherHandle ) {
		WaitForSingleObject( g_hooks.watcherHandle, 2000 );
		CloseHandle( g_hooks.watcherHandle );
		g_hooks.watcherHandle = nullptr;
	}

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

void __fastcall Hooks::Checkbox__Think(Checkbox* ecx) {
    // if it already existed, not inserted -> just call original
    if (!(g_checkboxInstances.find(ecx) != g_checkboxInstances.end())) {
        Log() << "Found checkbox " << ecx->label().c_str() << " at " << std::format("{:X}", (DWORD)ecx);
    }

    auto [it, inserted] = g_checkboxInstances.insert(ecx);

    return oCheckbox__Think( ecx );
}

void __fastcall Hooks::Dropdown__Think( Dropdown* ecx ) {
    // if it already existed, not inserted -> just call original 
    if ( !( g_dropdownInstances.find( ecx ) != g_dropdownInstances.end( ) ) ) {
        Log( ) << "Found dropdown " << ecx->label( ).c_str( ) << " at " << std::format( "{:X}", ( DWORD )ecx );
    }

    auto [it, inserted] = g_dropdownInstances.insert( ecx );

    return oDropdown__Think( ecx );
}

void __fastcall Hooks::MultiDropdown__Think(MultiDropdown* ecx) {
    uintptr_t base = reinterpret_cast<uintptr_t>(ecx);
    DumpMultiDropdownDebug(base);
    return oMultiDropdown__Think(ecx);
}

void __fastcall Hooks::Slider__Think( Slider* ecx ) {
    // if it already existed, not inserted -> just call original
    if ( !( g_sliderInstances.find( ecx ) != g_sliderInstances.end( ) ) ) {
        Log( ) << "Found slider " << ecx->label( ).c_str( ) << " at " << std::format( "{:X}", ( DWORD )ecx );

        /*
        if ( ecx->m_parent && ecx->m_parent->m_active_tab ) {
            Tab* tab = ecx->m_parent->m_active_tab;
            auto lol = tab->m_title; // you can log / debug here
        }
        */
    }

    auto [it, inserted] = g_sliderInstances.insert( ecx );

    return oSlider__Think( ecx );
}
