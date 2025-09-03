#pragma once
#include <vector>
#include <algorithm>'
#include <unordered_set>
#include <stdexcept>
#include <d3d9.h>
#include "typedef.h"

class Hooks {
public:
    void init( );
    void destroy( );

    // hooks
    using EndSceneFn = long( __thiscall* )( void*, IDirect3DDevice9* ) noexcept;
    inline static EndSceneFn EndSceneOriginal = nullptr;
    static long __stdcall EndScene( IDirect3DDevice9* device ) noexcept;

    using ResetFn = HRESULT( __thiscall* )( void*, IDirect3DDevice9*, D3DPRESENT_PARAMETERS* ) noexcept;
    inline static ResetFn ResetOriginal = nullptr;
    static HRESULT __stdcall Reset( IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params ) noexcept;

    using Checkbox__ThinkFn = void(__thiscall*)(Checkbox*);
    inline static Checkbox__ThinkFn oCheckbox__Think = nullptr;
    static void __fastcall Checkbox__Think(Checkbox* ecx);

    using Dropdown__ThinkFn = void( __thiscall* )( Dropdown* );
    inline static Dropdown__ThinkFn oDropdown__Think = nullptr;
    static void __fastcall Dropdown__Think( Dropdown* ecx );

    using MultiDropdown__ThinkFn = void( __thiscall* )( MultiDropdown* );
    inline static MultiDropdown__ThinkFn oMultiDropdown__Think = nullptr;
    static void __fastcall MultiDropdown__Think( MultiDropdown* ecx );

    using Slider__ThinkFn = void( __thiscall* )( Slider * );
    inline static Slider__ThinkFn oSlider__Think = nullptr;
    static void __fastcall Slider__Think( Slider* ecx );

    // funcs

    HANDLE watcherHandle = nullptr;
};

extern Hooks g_hooks;
extern std::unordered_set<Checkbox*> g_checkboxInstances;
extern std::unordered_set<Dropdown*> g_dropdownInstances;
extern std::unordered_set<MultiDropdown*> g_multiDropdownInstances;
extern std::unordered_set<Slider*> g_sliderInstances;
