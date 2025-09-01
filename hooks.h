#pragma once
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <d3d9.h>
#include "typedef.h"

class Hooks {
public:
    void init( );
    void destroy( );

    using EndSceneFn = long( __thiscall* )( void*, IDirect3DDevice9* ) noexcept;
    inline static EndSceneFn EndSceneOriginal = nullptr;
    static long __stdcall EndScene( IDirect3DDevice9* device ) noexcept;

    using ResetFn = HRESULT( __thiscall* )( void*, IDirect3DDevice9*, D3DPRESENT_PARAMETERS* ) noexcept;
    inline static ResetFn ResetOriginal = nullptr;
    static HRESULT __stdcall Reset( IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params ) noexcept;

    using CheckboxCtorFn = void( __thiscall* )( Checkbox* );
    inline static CheckboxCtorFn OriginalCheckboxCtor = nullptr;
    static void __fastcall CheckboxCtorHook( Checkbox* thisPtr );

    using Checkbox__ThinkFn = void(__thiscall*)(Checkbox*);
    inline static Checkbox__ThinkFn oCheckbox__Think = nullptr;
    static void __fastcall Checkbox__Think(Checkbox* ecx);

    HANDLE watcherHandle = nullptr;
};

extern Hooks g_hooks;
extern std::vector<Checkbox*> g_checkboxInstances;
