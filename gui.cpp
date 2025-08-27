#include "includes.h"
#include "gui.h"
#include "config.h"

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

// callback that allows ImGui to resize the std::string automatically
static int InputTextCallback( ImGuiInputTextCallbackData* data ) {
    if ( data->EventFlag == ImGuiInputTextFlags_CallbackResize ) {
        auto str = reinterpret_cast< std::string* >( data->UserData );
        str->resize( data->BufTextLen );
        data->Buf = str->data( );
    }
    return 0;
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

	ImGui::Begin( "hack", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize );

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
        CVariables::RAGE* rage = nullptr; // todo: add in weapon check functionality

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Aimbot", childSize( 0, 1, 1.f ), true );
        {

            struct MultiItem_t {
                std::string name;
                bool* value;
            };

            std::vector<MultiItem_t> hitboxes {
                { "Head", &rage->hitbox_head },
                { "Chest", &rage->hitbox_chest },
                { "Body", &rage->hitbox_body },
                { "Arms", &rage->hitbox_arms },
                { "Thighs", &rage->hitbox_thighs },
                { "Calfs", &rage->hitbox_calfs },
                { "Feet", &rage->hitbox_feet },
            };

            std::vector<MultiItem_t> multipoints{
                {"Head", &rage->multipoint_head },
                {"Chest", &rage->multipoint_chest },
                {"Body", &rage->multipoint_body },
                {"Arms", &rage->multipoint_arms },
                {"Thighs", &rage->multipoint_thighs },
                {"Calfs", &rage->multipoint_calfs },
                {"Feet", &rage->multipoint_feet },
            };

            ImGui::Checkbox( "Override default config", &rage->override_default_config);
            //ImGui::MultiSelect( "Hitboxes", )


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

        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Coloured Models", childSize( 1, 1, 1.f ), true );
        {

        }
        ImGui::EndChild( );
    }

    if ( selected_tab == 4 ) {
        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Miscellaneous", childSize( 0, 1 ), true );
        {
            ImGui::Checkbox( "sex1", &g_Vars.misc.sex1 );
            ImGui::Checkbox( "sex2", &g_Vars.misc.sex2 );
            ImGui::Checkbox( "sex3", &g_Vars.misc.sex3 );
        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Settings", childSize( 1, 1 ), true );
        {

        }
        ImGui::EndChild( );
    }

    if ( selected_tab == 5 ) {
        static std::vector<std::string> CFG_LIST;
        static bool INITIALISE_CONFIGS = true;
        static std::string config_name;

        if ( INITIALISE_CONFIGS ) {
            CFG_LIST = ConfigManager::GetConfigs( );
            INITIALISE_CONFIGS = false;

            if ( g_Vars.globals.m_iSelectedConfig >= 0 &&
                g_Vars.globals.m_iSelectedConfig < CFG_LIST.size( ) )
            {
                config_name = CFG_LIST[ g_Vars.globals.m_iSelectedConfig ];
            }
            else if ( !CFG_LIST.empty( ) ) {
                g_Vars.globals.m_iSelectedConfig = 0;
                config_name = CFG_LIST[ 0 ];
            }
            else {
                g_Vars.globals.m_iSelectedConfig = -1;
                config_name.clear( );
            }
        }

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Configurations", childSize( 0, 1 ), true );
        {
            ImVec2 available_space = ImGui::GetContentRegionAvail( );
            available_space.y -= ImGui::GetFrameHeightWithSpacing( ) + 5.0f;
            ImGui::BeginListBox( "##cfglist", available_space );
            for ( int i = 0; i < CFG_LIST.size( ); i++ ) {
                if ( ImGui::Selectable( CFG_LIST[ i ].c_str( ),
                    g_Vars.globals.m_iSelectedConfig == i ) ) {
                    g_Vars.globals.m_iSelectedConfig = i;
                    config_name = CFG_LIST[ i ]; // keep in sync
                }
            }
            ImGui::EndListBox( );

            // text input always shows the currently selected config name
            ImGui::InputText( "##cfg",
                config_name.data( ),
                config_name.capacity( ) + 1,
                ImGuiInputTextFlags_CallbackResize,
                InputTextCallback,
                &config_name );


        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Options", childSize( 1, 1 ), true );
        {
            auto it = std::find( CFG_LIST.begin( ), CFG_LIST.end( ), config_name );
            bool config_exists = ( it != CFG_LIST.end( ) );

            // --- Load ---
            if ( config_exists && ImGui::Button( "Load" ) ) {
                ConfigManager::ResetConfig( );
                ConfigManager::LoadConfig( config_name );
            }

            // --- Save ---
            if ( config_exists && ImGui::Button( "Save" ) ) {
                ConfigManager::SaveConfig( config_name );
            }

            // --- Delete ---
            if ( config_exists && !( config_name == XorStr( "default" ) ) ) {
                if ( ImGui::Button( "Delete config" ) ) {
                    ConfigManager::RemoveConfig( config_name );
                    CFG_LIST = ConfigManager::GetConfigs( );

                    if ( !CFG_LIST.empty( ) ) {
                        g_Vars.globals.m_iSelectedConfig = 0;
                        config_name = CFG_LIST[ 0 ];
                    }
                    else {
                        g_Vars.globals.m_iSelectedConfig = -1;
                        config_name.clear( );
                    }
                }
            }

            // --- Create ---
            if ( !config_name.empty( ) && !config_exists ) {
                if ( ImGui::Button( "Create config" ) ) {
                    ConfigManager::CreateConfig( config_name );
                    CFG_LIST = ConfigManager::GetConfigs( );

                    // re-select the new config
                    auto it = std::find( CFG_LIST.begin( ), CFG_LIST.end( ), config_name );
                    if ( it != CFG_LIST.end( ) ) {
                        g_Vars.globals.m_iSelectedConfig = std::distance( CFG_LIST.begin( ), it );
                    }
                }
            }

            if ( ImGui::Button( "Open config folder" ) ) {
                ConfigManager::OpenConfigFolder( );
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