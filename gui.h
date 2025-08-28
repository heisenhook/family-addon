#pragma once
#include <d3d9.h>
#include "dependencies/include/imgui/imgui.h"
#include "dependencies/include/imgui/imgui_impl_win32.h"
#include "dependencies/include/imgui/imgui_impl_dx9.h"

class Gui {
public:
	/* Prototypes */


	bool open = true;
	bool setup = false;
	int selected_tab;
	int selected_accuracy_tab = 0;

	HWND window = nullptr;
	WNDCLASSEX windowClass = { };
	WNDPROC originalWindowProcess = nullptr;
	
	// directx
	LPDIRECT3DDEVICE9 device = nullptr;
	LPDIRECT3D9 d3d9 = nullptr;

	bool SetupWindowClass( const char* windowClassName ) noexcept;
	void DestroyWindowClass( ) noexcept;

	bool SetupWindow( const char* windowName ) noexcept;
	void DestroyWindow( ) noexcept;

	bool SetupDirectX( ) noexcept;
	void DestroyDirectX( ) noexcept;
	
	// setup device
	void init( );

	void SetupMenu( LPDIRECT3DDEVICE9 device ) noexcept;
	void Destroy( ) noexcept;

	void Render( ) noexcept;
};

extern Gui g_gui;