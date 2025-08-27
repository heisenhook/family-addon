#include "gui.h"

#include "dependencies/include/imgui/imgui.h"
#include "dependencies/include/imgui/imgui_impl_win32.h"
#include "dependencies/include/imgui/imgui_impl_dx9.h"

#include <stdexcept>

Gui g_gui{ };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK WindowProcess( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ); // window process

bool Gui::SetupWindowClass( const char* windowClassName ) noexcept {
	// populate window class
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle( NULL );
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = NULL;

	// register
	if ( !RegisterClassEx( &windowClass ) )
		return false;

	return true;
}

void Gui::DestroyWindowClass( ) noexcept {
	UnregisterClass( windowClass.lpszClassName, windowClass.hInstance );
}

bool Gui::SetupWindow( const char* windowName ) noexcept {
	// create temp window
	window = CreateWindow( windowClass.lpszClassName, windowName, WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, windowClass.hInstance, 0 );

	if ( !window ) // if null
		return false; // return false (failed init)

	return true;
}

void Gui::DestroyWindow( ) noexcept {
	if ( window )
		::DestroyWindow( window );
}

bool Gui::SetupDirectX( ) noexcept {
	const auto handle = GetModuleHandle( "d3d9.dll" );

	if ( !handle )
		return false;

	using CreateFn = LPDIRECT3D9( __stdcall* )( UINT );

	const auto create = reinterpret_cast< CreateFn >( GetProcAddress( handle, "Direct3DCreate9" ) );

	if ( !create )
		return false;

	d3d9 = create( D3D_SDK_VERSION );

	if ( !d3d9 )
		return false;

	D3DPRESENT_PARAMETERS params = { };
	params.BackBufferWidth = 0;
	params.BackBufferHeight = 0;
	params.BackBufferFormat = D3DFMT_UNKNOWN;
	params.BackBufferCount = 0;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.MultiSampleQuality = NULL;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.Windowed = 1;
	params.EnableAutoDepthStencil = 0;
	params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	params.Flags = NULL;
	params.FullScreen_RefreshRateInHz = 0;
	params.PresentationInterval = 0;

	// create device
	if ( d3d9->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &device ) < 0 )
		return false;

	return true;
}

void Gui::DestroyDirectX( ) noexcept {
	if ( device ) {
		device->Release( );
		device = NULL;
	}

	if ( d3d9 ) {
		d3d9->Release( );
		d3d9 = NULL;
	}
}

void Gui::init( ) {
	if ( !SetupWindowClass( "hackClass001" ) )
		throw std::runtime_error( "Failed to create window class." );

	if ( !SetupWindow( "hack window" ) )
		throw std::runtime_error( "Failed to create window." );

	if ( !SetupDirectX( ) )
		throw std::runtime_error( "Failed to create device." );

	DestroyWindow( );
	DestroyWindowClass( );
}

void Gui::SetupMenu( LPDIRECT3DDEVICE9 device ) noexcept {
	auto params = D3DDEVICE_CREATION_PARAMETERS{ };
	device->GetCreationParameters( &params );

	window = params.hFocusWindow;
	originalWindowProcess = reinterpret_cast< WNDPROC >( SetWindowLongPtr( window, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( WindowProcess ) ) );

	ImGui::CreateContext( );
	ImGui::StyleColorsDark( );

	ImGui_ImplWin32_Init( window );
	ImGui_ImplDX9_Init( device );

	setup = true;
}

void Gui::Destroy( ) noexcept {
	ImGui_ImplDX9_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );

	// restore wnd proc
	SetWindowLongPtr( window, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( originalWindowProcess ) );

	DestroyDirectX( );
}

void Gui::Render( ) noexcept {
	ImGui_ImplDX9_NewFrame( );
	ImGui_ImplWin32_NewFrame( );

    static auto calculateChildWindowPosition = [ ]( int num, int max, float z = 1.f ) -> ImVec2 {
        return ImVec2( ImGui::GetWindowPos( ).x + 170 + ( ( ( ( ImGui::GetWindowSize( ).x - 170 ) / ( max + 1 ) ) ) * num ), ( ImGui::GetWindowPos( ).y + 8 ) ); // * ( ImGui::GetWindowSize( ).y / z )
        //return ImVec2( ImGui::GetWindowPos( ).x + 170 + ( ( ( ImGui::GetWindowSize( ).x - 170 ) / max ) * num, ImGui::GetWindowPos( ).y + 6 ) );
        };

    auto childSize = [ ]( int num, int max, float z = 1.f ) -> ImVec2 {
        return ImVec2( ( ( ImGui::GetWindowSize( ).x - 170 ) / ( max + 1 ) ) - 15, ( ImGui::GetWindowSize( ).y - 16 ) );
        };


	ImGui::NewFrame( );
	ImGui::SetNextWindowSize( ImVec2( 600, 400 ) );

	ImGui::Begin( "hack", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

	ImGui::BeginChild( "tabs", ImVec2{ 150, 0 }, true );
	{
		static constexpr const char* tabs[ ]{ "Rage", "Anti-Aim", "Legit", "Visuals", "Misc", "Config" };

		for ( std::size_t i{ }; i < IM_ARRAYSIZE( tabs ); ++i ) {
			if ( ImGui::Selectable( tabs[ i ], selected_tab == i ) ) {
				selected_tab = i;
			}
		}
	}
	ImGui::EndChild( );

    if ( selected_tab == 0 ) {
        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Aimbot", childSize( 0, 1, 1.f ), true );
        {
            /*
            ImGui::BeginTabBar("Settings#left_tabs_bar");

            // ericb 2017_07_21 : draw the tabs background BEFORE to fill it, to avoid a "colored overlay"
            ImGui::DrawTabsBackground();

            if (ImGui::AddTab("General")) {
                bool fullscreen = mGUICfg->fullScreen.value;
                if (ImGui::Checkbox("Fullscreen Mode", &fullscreen)) {
                    mGUICfg->toggleFullscreenMode = true;
                }
                if (ImGui::Checkbox("Enable Multisampling", &mGUICfg->enableMultisampling.value)) {
                    mGUICfg->settingsChanged = true;
                }

                if (ImGui::SliderInt("MSAA Count", (int*)&mGUICfg->multisampleCount.value, mGUICfg->multisampleCount.lowerLimit, mGUICfg->multisampleCount.upperLimit))
                    mGUICfg->settingsChanged = true;
            }
            if (ImGui::AddTab("GUI")) {
                ImGui::Text("Tab 2");
            }
            if (ImGui::AddTab("Tab Name")) {
                ImGui::Text("Tab 3");
            }
            if (ImGui::AddTab("Tab Name")) {
                ImGui::Text("Tab 4");
            }
            ImGui::EndTabBar();

            ImGui::Dummy(ImVec2(0, 20));

            ImGui::BeginTabBar("#Additional Parameters");
            float value = 0.0f;

            // ericb 2017_07_21 : draw the tabs background BEFORE to fill it, to avoid a "colored overlay"
            ImGui::DrawTabsBackground();

            if (ImGui::AddTab("Tab Name2")) {
                ImGui::SliderFloat("Slider", &value, 0, 1.0f);
            }
            if (ImGui::AddTab("Tab Name3")) {
                ImGui::Text("Tab 2");
            }
            if (ImGui::AddTab("Tab Name4")) {
                ImGui::Text("Tab 3");
            }
            if (ImGui::AddTab("Tab Name5")) {
                ImGui::Text("Tab 4");
            }
            ImGui::EndTabBar();
            */

        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Other", childSize( 1, 1, 1.f ), true );
        {

        }
        ImGui::EndChild( );
    }

    if ( selected_tab == 1 ) {
        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Anti-Aim", childSize( 0, 1 ), true );
        {

        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Fake lag", childSize( 1, 1, 0.5 ), true );
        {

        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Other", childSize( 1, 1, 0.5 ), true );
        {

        }
        ImGui::EndChild( );
    }

    if ( selected_tab == 2 ) {

    }

    if ( selected_tab == 3 ) {
        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Players", childSize( 0, 1, 1.f ), true );
        {
            ImGui::Checkbox( "Bounding Box", &g_config->b[ _( "esp_bounding_box" ) ] );
            ImGui::Checkbox( "Name", &g_config->b[ _( "esp_name" ) ] );
            ImGui::Checkbox( "Health bar", &g_config->b[ _( "esp_health_bar" ) ] );
            ImGui::Checkbox( "Weapon Name", &g_config->b[ _( "esp_wpn_name" ) ] );
            ImGui::Checkbox( "Weapon Icon", &g_config->b[ _( "esp_wpn_icon" ) ] );

        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Coloured Models", childSize( 1, 1, 1.f ), true );
        {
            ImGui::Checkbox( "Enemy", &g_config->b[ _( "chams_enemy" ) ] );
            ImGui::ColorEdit4( "##Enemy", g_config->c[ _( "chams_enemy_col" ) ] );
            ImGui::Checkbox( "Enemy behind walls", &g_config->b[ _( "chams_enemy_xqz" ) ] );
            ImGui::ColorEdit4( "##Enemy behind walls", g_config->c[ _( "chams_enemy_xqz_col" ) ] );
        }
        ImGui::EndChild( );
    }

    if ( selected_tab == 4 ) {
        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Miscellaneous", childSize( 0, 1 ), true );
        {
            ImGui::Checkbox( "Bunnyhop", &g_config->b[ _( "move_bhop" ) ] );
        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Settings", childSize( 1, 1 ), true );
        {

        }
        ImGui::EndChild( );
    }

    if ( m_selected_tab == 5 ) {
        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Configurations", childSize( 0, 1 ), true );
        {
            static char cfgname[ 26 ];

            ImGui::ListBoxHeader( "##cfglist", ImVec2( 225, 400 ) );
            for ( auto cfg : g_config->configs )
                if ( ImGui::Selectable( cfg.c_str( ), cfg == g_config->selected_cfg ) ) {
                    g_config->selected_cfg = cfg;
                    strcpy( g_config->input, cfg.c_str( ) );
                    log_print( "g_config->selected_cfg = {}", cfg );
                }

            ImGui::ListBoxFooter( );

            //ImGui::InputText( "##cfg", strcpy( new char[ g_config->input.length( ) + 1 ], g_config->input.c_str( ) ), 128 );

            if ( ImGui::InputText( "##cfg", g_config->input, 128 ) )
                g_config->selected_cfg = std::string( g_config->input );
        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Options", childSize( 1, 1 ), true );
        {

            if ( ImGui::Button( "Load" ) ) {
                g_config->load( );
            }

            if ( ImGui::Button( "Save" ) ) {
                g_config->save( );
            }

        }
        ImGui::EndChild( );
    }

    ImGui::SameLine( );

	ImGui::End( );

	ImGui::EndFrame( );
	ImGui::Render( );
	ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData( ) );
}

LRESULT CALLBACK WindowProcess( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	// toggle menu
	if ( GetAsyncKeyState( VK_INSERT ) & 1 )
		g_gui.open = !g_gui.open;

	// pass msg to imgui
	if ( g_gui.open && ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
		return 1L; // return & do not call original window process

	return CallWindowProc( g_gui.originalWindowProcess, hWnd, msg, wParam, lParam );
}