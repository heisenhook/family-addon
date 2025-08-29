#include "includes.h"
#include "gui.h"
#include "config.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include "imgui_internal.h"

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

    ImGuiIO& io = ImGui::GetIO( );
    io.Fonts->AddFontFromFileTTF( "C:\\Windows\\Fonts\\Tahoma.ttf", 14.0f );
    io.FontDefault = io.Fonts->Fonts.back( );

    ImGuiStyle& style = ImGui::GetStyle( );
    
    style.Colors[ ImGuiCol_Text ] = ImVec4( 0.89f, 0.89f, 0.89f, 1.f );

    /*
    // Background of slider
    style.Colors[ ImGuiCol_FrameBg ] = ImVec4( 0.15f, 0.15f, 0.15f, 1.0f ); 
    // Filled portion
    style.Colors[ ImGuiCol_FrameBgActive ] = ImVec4( 0.30f, 0.60f, 1.0f, 1.0f );
    // Hovered portion
    style.Colors[ ImGuiCol_FrameBgHovered ] = ImVec4( 0.25f, 0.50f, 0.9f, 1.0f );
    // Knob
    style.Colors[ ImGuiCol_SliderGrab ] = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
    style.Colors[ ImGuiCol_SliderGrabActive ] = ImVec4( 1.0f, 0.8f, 0.2f, 1.0f );
    */

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

struct MultiItem_t {
    std::string name;
    bool* value;
};

inline bool ImGui_Dropdown( const char* label, const std::vector<std::string>& items, int* selected )
{
    bool changed = false;

    // Preview text
    std::string preview = ( *selected >= 0 && *selected < items.size( ) ) ? items[ *selected ] : "None";

    ImGui::TextUnformatted( label );
    ImGui::SetNextItemWidth( 160 );
    if ( ImGui::BeginCombo( ( std::string( "##" ) + label ).c_str( ), preview.c_str( ) ) )
    {
        for ( int i = 0; i < items.size( ); i++ )
        {
            bool is_selected = ( *selected == i );
            if ( ImGui::Selectable( items[ i ].c_str( ), is_selected ) )
            {
                *selected = i;
                changed = true;
            }
            if ( is_selected )
                ImGui::SetItemDefaultFocus( );
        }
        ImGui::EndCombo( );
    }

    return changed;
}

inline bool ImGui_MultiSelect( const char* label, std::vector<MultiItem_t>& items )
{
    bool changed = false;

    // Build preview string
    std::string preview;
    for ( auto& item : items )
        if ( *item.value )
            preview += item.name + ", ";

    if ( !preview.empty( ) )
        preview.pop_back( ), preview.pop_back( ); // remove trailing ", "
    else
        preview = "None";

    ImGui::TextUnformatted( label );
    ImGui::SetNextItemWidth( 160 );
    if ( ImGui::BeginCombo( ( std::string( "##" ) + label ).c_str( ), preview.c_str( ) ) )
    {
        for ( auto& item : items )
        {
            if ( ImGui::Selectable( item.name.c_str( ), *item.value, ImGuiSelectableFlags_DontClosePopups) )
            {
                *item.value = !*item.value;
                changed = true;
            }
            if ( *item.value )
                ImGui::SetItemDefaultFocus( );
        }
        ImGui::EndCombo( );
    }

    return changed;
}

inline bool SliderInt( const char* label, int* v, int v_min, int v_max, const char* format = "%d" ) {
    ImGui::TextUnformatted( label );

    ImGui::SetNextItemWidth( 174 );

    // Invisible interaction area
    ImGui::InvisibleButton( ( std::string( "##" ) + label ).c_str( ), ImVec2( 174, 6 ) );
    bool hovered = ImGui::IsItemHovered( );
    bool active = ImGui::IsItemActive( );

    // Handle mouse input
    if ( active && ImGui::GetIO( ).MouseDown[ 0 ] ) {
        ImVec2 pos = ImGui::GetItemRectMin( );
        ImVec2 size = ImGui::GetItemRectSize( );
        float t = ( ImGui::GetIO( ).MousePos.x - pos.x ) / size.x;
        t = ( t < 0.f ) ? 0.f : ( t > 1.f ) ? 1.f : t;
        *v = int( v_min + t * ( v_max - v_min ) );
    }

    // Draw the bar
    ImDrawList* dl = ImGui::GetWindowDrawList( );
    ImVec2 min = ImGui::GetItemRectMin( );
    ImVec2 max = ImGui::GetItemRectMax( );
    ImU32 bg_col = IM_COL32( 50, 50, 50, 255 );
    ImU32 fill_col = IM_COL32( 0, 180, 255, 255 );

    dl->AddRectFilled( min, max, bg_col, 0.f );  // background
    float t = float( *v - v_min ) / float( v_max - v_min );
    ImVec2 fill_max = ImVec2( min.x + ( max.x - min.x ) * t, max.y );
    dl->AddRectFilled( min, fill_max, fill_col, 0.f ); // fill

    // Draw value text with background for readability
    char buf[ 32 ];
    sprintf( buf, format, *v );
    ImVec2 text_size = ImGui::CalcTextSize( buf );
    ImVec2 text_pos = ImVec2( fill_max.x - text_size.x / 2, min.y - 4 );

    ImU32 text_col = IM_COL32( 255, 255, 255, 255 );   // White text
    ImU32 outline_col = IM_COL32( 0, 0, 0, 100 );      // Black outline

    // Draw outline by drawing text in 8 directions around original position
    for ( int x = -1; x <= 1; x++ )
    {
        for ( int y = -1; y <= 1; y++ )
        {
            if ( x != 0 || y != 0 )
                dl->AddText( ImVec2( text_pos.x + x, text_pos.y + y ), outline_col, buf );
        }
    }

    // Draw main text on top
    dl->AddText( text_pos, text_col, buf );

    return active;
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
	ImGui::SetNextWindowSize( ImVec2( 600, 500 ) );

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
        CVariables::RAGE* rage = &g_Vars.rage_default; // todo: add in weapon check functionality

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 0, 1 ) );
        ImGui::BeginChild( "Aimbot", childSize( 0, 1, 1.f ), true );
        {

            std::vector<MultiItem_t> hitboxes {
                { "Head", &rage->hitbox_head },
                { "Chest", &rage->hitbox_chest },
                { "Body", &rage->hitbox_body },
                { "Arms", &rage->hitbox_arms },
                { "Thighs", &rage->hitbox_thighs },
                { "Calfs", &rage->hitbox_calfs },
                { "Feet", &rage->hitbox_feet },
            };

            std::vector<MultiItem_t> multipoints {
                {"Head", &rage->multipoint_head },
                {"Chest", &rage->multipoint_chest },
                {"Body", &rage->multipoint_body },
                {"Arms", &rage->multipoint_arms },
                {"Thighs", &rage->multipoint_thighs },
                {"Calfs", &rage->multipoint_calfs },
                {"Feet", &rage->multipoint_feet },
            };

            std::vector<std::string> multipoint_intensity {
                "Normal",
                "High",
                "Very high"
            };

            ImGui::Checkbox( "Override default config", &rage->override_default_config);
            //ImGui::MultiSelect( "Hitboxes", )

            

            ImGui_MultiSelect( "Hitboxes", hitboxes );
            ImGui_MultiSelect( "Multipoints", multipoints );
            ImGui_Dropdown( "Multipoint Intensity", multipoint_intensity, &rage->multipoint_intensity );
            SliderInt( "Pointscale", &rage->pointscale, 50, 100 );
            SliderInt( "Maximum aim-height", &rage->maximum_aim_height, 50, 100 );
            ImGui::Checkbox( "Seperate point-scale", &rage->seperate_pointscale );

            if ( rage->seperate_pointscale ) {
                SliderInt( "Head pointscale", &rage->pointscale_head, 50, 100);
                SliderInt( "Limb pointscale", &rage->pointscale_limbs, 50, 100);
            }

            SliderInt( "Minimal hit damage", &rage->minimum_damage, 1, 100 );

        }
        ImGui::EndChild( );

        ImGui::SetNextWindowPos( calculateChildWindowPosition( 1, 1 ) );
        ImGui::BeginChild( "Other", childSize( 1, 1, 1.f ), true );
        {

            SliderInt( "Minimal hitchance", &rage->minimum_hitchance, 1, 100 );
            SliderInt( "Prefer hitchance", &rage->prefer_hitchance, -100, 100 );
            SliderInt( "Prefer body aim", &rage->prefer_baim, 0, 100 );
            SliderInt( "Minimal assessed damage", &rage->minimal_assessed_damage, 1, 100 );

            std::vector<MultiItem_t> autostop{
                {"Early", &rage->autostop_early },
                {"Stop", &rage->autostop_stop },
                {"Jump-scout", &rage->autostop_jumpscout },
                {"Auto-scope", &rage->autostop_autoscope },
                {"Delay for fake lag", &rage->autostop_delay_fakelag },
                {"Delay for spread", &rage->autostop_delay_spread },
                {"Delay for taser", &rage->autostop_delay_taser },
            };

            ImGui_MultiSelect( "Automatic stop", autostop );

            ImGui_Dropdown( "Accuracy", { "Fake angles", "Lag compensation", "Override aimbot", "Override accuracy" }, &g_Vars.rage.accuracy );

            // Fake angles
            if ( g_Vars.rage.accuracy == 0 ) {
                ImGui_Dropdown( "Standing body aim", { "Off", "Low", "Medium", "High", "Very high" }, &rage->standing_baim);
                ImGui_Dropdown( "Strict standing body aim", { "Off", "Low", "Medium", "High", "Very high" }, &rage->strict_standing_baim);
                ImGui_Dropdown( "In-air body aim", { "Off", "Low", "Medium", "High", "Very high" }, &rage->air_baim);
                SliderInt( "Limit in-air pointscale", &rage->limit_air_pointscale, 0, 100 );
                ImGui_Dropdown( "Strict in-air body aim", { "Off", "Low", "Medium", "High", "Very high" }, &rage->strict_air_baim);
            }

            // Lag compensation
            if ( g_Vars.rage.accuracy == 1 ) {
                ImGui_Dropdown( "Avoid body edges", { "Off", "Low", "Medium", "High", }, &rage->avoid_body_edges );
                SliderInt( "Prefer fake body overlap", &rage->prefer_fakebody_overlap, 0, 100 );
                ImGui_Dropdown( "Head accuracy mode", { "Below normal", "Normal", "Strict", }, &rage->avoid_body_edges );
                SliderInt( "Prefer fake head overlap", &rage->prefer_fakehead_overlap, 0, 100 );
            }

            // Override aimbot
            if ( g_Vars.rage.accuracy == 2 ) {
                SliderInt( "Minimal damage primary override", &rage->minimum_damage_override_primary, 0, 100 );
                SliderInt( "Minimal damage secondary override", &rage->minimum_damage_override_secondary, 0, 100 );
                ImGui::Checkbox( "Override hitchance", &rage->minimum_damage_override_hitchance );
                if ( rage->minimum_damage_override_hitchance ) {
                    SliderInt( "Hitchance primary override", &rage->minimum_hitchance_override_primary, 0, 100 );
                    SliderInt( "Hitchance secondary override", &rage->minimum_damage_override_secondary, 0, 100 );
                } 
            }

            // Override accuracy
            if ( g_Vars.rage.accuracy == 3 ) {
                ImGui::Checkbox( "Reduce hitchance to avoid delay", &rage->reduce_hitchance );

                if ( rage->reduce_hitchance ) {
                    SliderInt( "Reduced hitchance", &rage->reduce_hitchance_value, 0, 100 );
                    SliderInt( "Period to wait until reducing", &rage->reduce_hitchance_delay, 0, 250 );
                    ImGui::Checkbox( "Reduce hitchance with damage override", &rage->reduce_hitchance_damage_override );
                }

                std::vector<MultiItem_t> autostopflags{
                    {"Fallback", &rage->autostopflags_fallback },
                    {"Avoid locking movement", &rage->autostopflags_avoid_locking_movement },
                    {"Early on lag-delayed", &rage->autostopflags_early_lag_delayed },
                    {"Early on normal-peek", &rage->autostopflags_early_normal_peek },
                    {"Early on aggressive-peek", &rage->autostopflags_early_aggresive_peek },
                };

                ImGui_MultiSelect( "Automatic stop flags", autostopflags );

                ImGui_Dropdown( "Passive delay on peek", { "Off", "When safe", "Always" }, & rage->passive_delay_on_peek );
                ImGui_Dropdown( "Force delay on peek", { "Off", "When safe", "Always" }, & rage->force_delay_on_peek );

            }

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
            ImGui::Checkbox( "Disable adaptive weapons", &g_Vars.misc.disable_adaptive_weapons );
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