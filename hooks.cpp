#include "includes.h"
#include "memory2.h"

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

uintptr_t familyhookBase;

void Hooks::init( ) {
    PE::Module m_serverbrowser_dll;

    while (!m_serverbrowser_dll) {
        m_serverbrowser_dll = PE::GetModule(HASH("serverbrowser.dll"));
        if (!m_serverbrowser_dll)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    gLogger.attach("family-addon");
    g_gui.init();

    familyhookBase = reinterpret_cast<uintptr_t>(GetModuleHandle("HisHolySpiritOurLordAndSaviorJesusChrist.dll"));

	if ( MH_Initialize( ) )
		throw std::runtime_error( "unable to init minhook" );

	if ( MH_CreateHook( g_memory.Get( g_gui.device, 42 ), &EndScene, reinterpret_cast< void** >( &EndSceneOriginal ) ) )
		throw std::runtime_error( "Unable to hook EndScene()" );

	if ( MH_CreateHook( g_memory.Get( g_gui.device, 16 ), &Reset, reinterpret_cast< void** >( &ResetOriginal ) ) )
		throw std::runtime_error( "Unable to hook Reset()" );

    if (MH_CreateHook(g_memory.PatternScan("HisHolySpiritOurLordAndSaviorJesusChrist.dll",
        "55 8B EC 83 E4 F8 81 EC 00 05 00 00"), &hkAimbotThink, reinterpret_cast<void**>(&oAimbotThink)))
        throw std::runtime_error("Unable to hook Aimbot::Think()");

    if (MH_CreateHook(g_memory.PatternScan("HisHolySpiritOurLordAndSaviorJesusChrist.dll",
        "55 8B EC 83 EC 18 53 56 8B F1"), &hkFormDraw, reinterpret_cast<void**>(&oFormDraw)))
        throw std::runtime_error("Unable to hook Form::draw()");

    //if (MH_CreateHook(g_memory.PatternScan("HisHolySpiritOurLordAndSaviorJesusChrist.dll",
    //    "55 8B EC 81 EC 98 01 00 00"), &hkConfigSave, reinterpret_cast<void**>(&oConfigSave)))
    //    throw std::runtime_error("Unable to hook Config::Save()");

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

std::vector<Tab*> tabs;
std::vector<Element*> elements;
Form* form = nullptr;

void __fastcall Hooks::hkAimbotThink(void* ecx) {

    // do weapon configs here

    return oAimbotThink(ecx);
}

void* __fastcall Hooks::hkFormDraw(void* ecx) {
    if (!form)
        form = reinterpret_cast<Form*>(familyhookBase + 0x141ED0);

    g_gui.open = form->m_open;

    if (tabs.size() == 0) {
        if (form) {
            tabs = form->GetTabs();
            for (auto tab : tabs) {
                if (tab) {
                    for (auto e : tab->GetElements()) {
                        elements.push_back(e);
                    }
                }
            }
        }
    }

    return oFormDraw(ecx);
}

//const char* Hooks::hkConfigSave(void* a1, void* a2, void* a3, void* a4) {
//    return oConfigSave(a1, a2, a3, a4);
//}