#pragma once
#include <d3d9.h>

class Gui {
public:
	bool open = true;
	bool setup = false;
	int selected_tab;

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