#pragma once

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
};

extern Hooks g_hooks;