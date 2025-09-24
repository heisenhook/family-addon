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
uintptr_t familyhookBase;

void Hooks::init( ) {
    PE::Module m_serverbrowser_dll;

    while (!m_serverbrowser_dll) {
        m_serverbrowser_dll = PE::GetModule(HASH("serverbrowser.dll"));
        if (!m_serverbrowser_dll)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    g_csgo.m_kernel32_dll = PE::GetModule(HASH("kernel32.dll"));
	g_csgo.m_user32_dll = PE::GetModule(HASH("user32.dll"));
	g_csgo.m_shell32_dll = PE::GetModule(HASH("shell32.dll"));
	g_csgo.m_shlwapi_dll = PE::GetModule(HASH("shlwapi.dll"));
	g_csgo.m_client_dll = PE::GetModule(HASH("client.dll"));
	g_csgo.m_engine_dll = PE::GetModule(HASH("engine.dll"));
	g_csgo.m_vstdlib_dll = PE::GetModule(HASH("vstdlib.dll"));
	g_csgo.m_tier0_dll = PE::GetModule(HASH("tier0.dll"));

	// import winapi functions.
	g_winapi.WideCharToMultiByte = PE::GetExport(g_csgo.m_kernel32_dll, HASH("WideCharToMultiByte")).as< WinAPI::WideCharToMultiByte_t >();
	g_winapi.MultiByteToWideChar = PE::GetExport(g_csgo.m_kernel32_dll, HASH("MultiByteToWideChar")).as< WinAPI::MultiByteToWideChar_t >();
	g_winapi.GetTickCount = PE::GetExport(g_csgo.m_kernel32_dll, HASH("GetTickCount")).as< WinAPI::GetTickCount_t >();
	g_winapi.VirtualProtect = PE::GetExport(g_csgo.m_kernel32_dll, HASH("VirtualProtect")).as< WinAPI::VirtualProtect_t >();
	g_winapi.VirtualQuery = PE::GetExport(g_csgo.m_kernel32_dll, HASH("VirtualQuery")).as< WinAPI::VirtualQuery_t >();
	g_winapi.CreateDirectoryA = PE::GetExport(g_csgo.m_kernel32_dll, HASH("CreateDirectoryA")).as< WinAPI::CreateDirectoryA_t >();
	g_winapi.SetWindowLongA = PE::GetExport(g_csgo.m_user32_dll, HASH("SetWindowLongA")).as< WinAPI::SetWindowLongA_t >();
	g_winapi.CallWindowProcA = PE::GetExport(g_csgo.m_user32_dll, HASH("CallWindowProcA")).as< WinAPI::CallWindowProcA_t >();
	g_winapi.SHGetFolderPathA = PE::GetExport(g_csgo.m_shell32_dll, HASH("SHGetFolderPathA")).as< WinAPI::SHGetFolderPathA_t >();
	g_winapi.PathAppendA = PE::GetExport(g_csgo.m_shlwapi_dll, HASH("PathAppendA")).as< WinAPI::PathAppendA_t >();

	// run interface collection code.
	Interfaces interfaces{};

	// get interface pointers.
	g_csgo.m_client = interfaces.get< CHLClient* >(HASH("VClient"));
	/*g_csgo.m_cvar = interfaces.get< ICvar* >(HASH("VEngineCvar"));
	g_csgo.m_engine = interfaces.get< IVEngineClient* >(HASH("VEngineClient"));
	g_csgo.m_entlist = interfaces.get< IClientEntityList* >(HASH("VClientEntityList"));
	g_csgo.m_input_system = interfaces.get< IInputSystem* >(HASH("InputSystemVersion"));
	g_csgo.m_surface = interfaces.get< ISurface* >(HASH("VGUI_Surface"));
	g_csgo.m_panel = interfaces.get< IPanel* >(HASH("VGUI_Panel"));
	g_csgo.m_engine_vgui = interfaces.get< IEngineVGui* >(HASH("VEngineVGui"));
	g_csgo.m_prediction = interfaces.get< CPrediction* >(HASH("VClientPrediction"));
	g_csgo.m_engine_trace = interfaces.get< IEngineTrace* >(HASH("EngineTraceClient"));
	g_csgo.m_game_movement = interfaces.get< CGameMovement* >(HASH("GameMovement"));
	g_csgo.m_render_view = interfaces.get< IVRenderView* >(HASH("VEngineRenderView"));
	g_csgo.m_model_render = interfaces.get< IVModelRender* >(HASH("VEngineModel"));
	g_csgo.m_material_system = interfaces.get< IMaterialSystem* >(HASH("VMaterialSystem"));
	g_csgo.m_studio_render = interfaces.get< CStudioRenderContext* >(HASH("VStudioRender"));
	g_csgo.m_model_info = interfaces.get< IVModelInfo* >(HASH("VModelInfoClient"));
	g_csgo.m_debug_overlay = interfaces.get< IVDebugOverlay* >(HASH("VDebugOverlay"));
	g_csgo.m_phys_props = interfaces.get< IPhysicsSurfaceProps* >(HASH("VPhysicsSurfaceProps"));
	g_csgo.m_game_events = interfaces.get< IGameEventManager2* >(HASH("GAMEEVENTSMANAGER"), 1);
	g_csgo.m_match_framework = interfaces.get< CMatchFramework* >(HASH("MATCHFRAMEWORK_"));
	g_csgo.m_localize = interfaces.get< ILocalize* >(HASH("Localize_"));
	g_csgo.m_networkstringtable = interfaces.get< INetworkStringTableContainer* >(HASH("VEngineClientStringTable"));
	g_csgo.m_sound = interfaces.get< IEngineSound* >(HASH("IEngineSoundClient"));

	// convars.
	g_csgo.clear = g_csgo.m_cvar->FindVar(HASH("clear"));
	g_csgo.toggleconsole = g_csgo.m_cvar->FindVar(HASH("toggleconsole"));
	g_csgo.name = g_csgo.m_cvar->FindVar(HASH("name"));
	g_csgo.sv_maxunlag = g_csgo.m_cvar->FindVar(HASH("sv_maxunlag"));
	g_csgo.sv_gravity = g_csgo.m_cvar->FindVar(HASH("sv_gravity"));
	g_csgo.sv_jump_impulse = g_csgo.m_cvar->FindVar(HASH("sv_jump_impulse"));
	g_csgo.sv_enablebunnyhopping = g_csgo.m_cvar->FindVar(HASH("sv_enablebunnyhopping"));
	g_csgo.sv_airaccelerate = g_csgo.m_cvar->FindVar(HASH("sv_airaccelerate"));
	g_csgo.sv_friction = g_csgo.m_cvar->FindVar(HASH("sv_friction"));
	g_csgo.sv_stopspeed = g_csgo.m_cvar->FindVar(HASH("sv_stopspeed"));
	g_csgo.cl_interp = g_csgo.m_cvar->FindVar(HASH("cl_interp"));
	g_csgo.cl_interp_ratio = g_csgo.m_cvar->FindVar(HASH("cl_interp_ratio"));
	g_csgo.cl_updaterate = g_csgo.m_cvar->FindVar(HASH("cl_updaterate"));
	g_csgo.cl_cmdrate = g_csgo.m_cvar->FindVar(HASH("cl_cmdrate"));
	g_csgo.cl_lagcompensation = g_csgo.m_cvar->FindVar(HASH("cl_lagcompensation"));
	g_csgo.mp_teammates_are_enemies = g_csgo.m_cvar->FindVar(HASH("mp_teammates_are_enemies"));
	g_csgo.weapon_debug_spread_show = g_csgo.m_cvar->FindVar(HASH("weapon_debug_spread_show"));
	g_csgo.molotov_throw_detonate_time = g_csgo.m_cvar->FindVar(HASH("molotov_throw_detonate_time"));
	g_csgo.weapon_molotov_maxdetonateslope = g_csgo.m_cvar->FindVar(HASH("weapon_molotov_maxdetonateslope"));
	g_csgo.weapon_recoil_scale = g_csgo.m_cvar->FindVar(HASH("weapon_recoil_scale"));
	g_csgo.view_recoil_tracking = g_csgo.m_cvar->FindVar(HASH("view_recoil_tracking"));
	g_csgo.cl_fullupdate = g_csgo.m_cvar->FindVar(HASH("cl_fullupdate"));
	g_csgo.r_DrawSpecificStaticProp = g_csgo.m_cvar->FindVar(HASH("r_DrawSpecificStaticProp"));
	g_csgo.cl_crosshair_sniper_width = g_csgo.m_cvar->FindVar(HASH("cl_crosshair_sniper_width"));
	g_csgo.hud_scaling = g_csgo.m_cvar->FindVar(HASH("hud_scaling"));
	g_csgo.sv_clip_penetration_traces_to_players = g_csgo.m_cvar->FindVar(HASH("sv_clip_penetration_traces_to_players"));
	g_csgo.weapon_accuracy_shotgun_spread_patterns = g_csgo.m_cvar->FindVar(HASH("weapon_accuracy_shotgun_spread_patterns"));
	g_csgo.net_showfragments = g_csgo.m_cvar->FindVar(HASH("net_showfragments"));

	// hehe xd.
	g_csgo.name->m_callbacks.RemoveAll();
	//cl_lagcompensation->m_callbacks.RemoveAll( );
	//cl_lagcompensation->m_flags &= ~FCVAR_NOT_CONNECTED;

	// classes by sig.
	g_csgo.m_move_helper = pattern::find(g_csgo.m_client_dll, XOR("8B 0D ? ? ? ? 8B 46 08 68")).add(2).get< IMoveHelper* >(2);
	g_csgo.m_cl = **reinterpret_cast<CClientState***> ((*reinterpret_cast<uintptr_t**> (g_csgo.m_engine))[12] + 0x10);
	g_csgo.m_game = pattern::find(g_csgo.m_engine_dll, XOR("A1 ? ? ? ? B9 ? ? ? ? FF 75 08 FF 50 34")).add(1).get< CGame* >();
	g_csgo.m_render = pattern::find(g_csgo.m_engine_dll, XOR("A1 ? ? ? ? B9 ? ? ? ? FF 75 0C FF 75 08 FF 50 0C")).add(1).get< CRender* >();
	g_csgo.m_shadow_mgr = pattern::find(g_csgo.m_client_dll, XOR("A1 ? ? ? ? FF 90 ? ? ? ? 6A 00")).add(1).get().as< IClientShadowMgr* >();
	g_csgo.m_view_render = pattern::find(g_csgo.m_client_dll, XOR("8B 0D ? ? ? ? 8B 01 FF 50 4C 8B 06")).add(2).get< CViewRender* >(2);
	// m_entity_listeners   = pattern::find( m_client_dll, XOR( "B9 ? ? ? ? E8 ? ? ? ? 5E 5D C2 04" ) ).add( 0x1 ).get< IClientEntityListener** >( 2 );
	g_csgo.m_hud = pattern::find(g_csgo.m_client_dll, XOR("B9 ? ? ? ? 0F 94 C0 0F B6 C0 50 68")).add(0x1).get().as< CHud* >();
	g_csgo.m_gamerules = pattern::find(g_csgo.m_client_dll, XOR("8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 75 6B")).add(0x2).get< C_CSGameRules* >();
	g_csgo.m_beams = pattern::find(g_csgo.m_client_dll, XOR("8D 04 24 50 A1 ? ? ? ? B9")).add(0x5).get< IViewRenderBeams* >();
	g_csgo.m_mem_alloc = PE::GetExport(g_csgo.m_tier0_dll, HASH("g_pMemAlloc")).get< IMemAlloc* >();
	g_csgo.GetGlowObjectManager = pattern::find(g_csgo.m_client_dll, XOR("A1 ? ? ? ? A8 01 75 4B")).as< CSGO::GetGlowObjectManager_t >();
	g_csgo.m_glow = g_csgo.GetGlowObjectManager();
	g_csgo.m_hookable_cl = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(g_csgo.m_cl) + 0x8);

	// classes by offset from virtual.
	g_csgo.m_globals = util::get_method(g_csgo.m_client, CHLClient::INIT).add(0x1b).get< CGlobalVarsBase* >(2);
	g_csgo.m_client_mode = util::get_method(g_csgo.m_client, CHLClient::HUDPROCESSINPUT).add(0x5).get< IClientMode* >(2);
	g_csgo.m_input = util::get_method(g_csgo.m_client, CHLClient::INACTIVATEMOUSE).at< CInput* >(0x1);

	// functions.
	g_csgo.MD5_PseudoRandom = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 70 6A 58")).as< CSGO::MD5_PseudoRandom_t >();
	g_csgo.SetAbsAngles = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8"));
	g_csgo.InvalidateBoneCache = pattern::find(g_csgo.m_client_dll, XOR("80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81"));
	g_csgo.LockStudioHdr = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 51 53 8B D9 56 57 8D B3"));
	g_csgo.SetAbsOrigin = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 51 53 56 57 8B F1"));
	g_csgo.IsBreakableEntity = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 51 56 8B F1 85 F6 74 68 83 BE")).as< CSGO::IsBreakableEntity_t >();
	g_csgo.SetAbsVelocity = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 0C 53 56 57 8B 7D 08 8B F1"));
	g_csgo.AngleMatrix = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 8B 07 89 46 0C")).rel32(0x1).as< CSGO::AngleMatrix_t >();
	g_csgo.ComputeHitboxSurroundingBox = pattern::find(g_csgo.m_client_dll, XOR("E9 ? ? ? ? 32 C0 5D")).rel32(0x1);
	g_csgo.GetSequenceActivity = pattern::find(g_csgo.m_client_dll, XOR("53 56 8B F1 8B DA 85 F6 74 55"));
	g_csgo.LoadFromBuffer = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 88 44 24 0F 8B 56 FC")).rel32(0x1).as< CSGO::LoadFromBuffer_t >();
	g_csgo.ServerRankRevealAll = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 8B 0D ? ? ? ? 68")).as< CSGO::ServerRankRevealAll_t >();
	g_csgo.HasC4 = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 38 83")).rel32(0x1);
	g_csgo.InvalidatePhysicsRecursive = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 89 5E 18")).rel32(0x1);
	g_csgo.IsReady = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 59 C2 08 00 51 E8")).rel32(0x1).as< CSGO::IsReady_t >();
	g_csgo.ShowAndUpdateSelection = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? A1 ? ? ? ? F3 0F 10 40 ? C6 83")).rel32(0x1).as< CSGO::ShowAndUpdateSelection_t >();
	g_csgo.GetEconItemView = pattern::find(g_csgo.m_client_dll, XOR("8B 81 ? ? ? ? 81 C1 ? ? ? ? FF 50 04 83 C0 40 C3")).as< CSGO::GetEconItemView_t >();
	g_csgo.GetStaticData = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 51 56 57 8B F1 E8 ? ? ? ? 0F B7 8E")).as< CSGO::GetStaticData_t >();
	//TEFireBullets = pattern::find( m_client_dll, XOR( "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 66 A3" ) ).add( 0x2 ).to( );
	g_csgo.BeamAlloc = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 8B F0 85 F6 74 7C")).rel32< CSGO::BeamAlloc_t >(0x1);
	g_csgo.SetupBeam = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 8B 07 33 C9")).rel32< CSGO::SetupBeam_t >(0x1);
	g_csgo.ClearNotices = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 8B F0 85 F6 74 19")).rel32< CSGO::ClearNotices_t >(0x1);
	g_csgo.AddListenerEntity = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 8B 0D ? ? ? ? 33 C0 56 85 C9 7E 32 8B 55 08 8B 35")).as< CSGO::AddListenerEntity_t >();
	g_csgo.GetShotgunSpread = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? EB 38 83 EC 08")).rel32< CSGO::GetShotgunSpread_t >(1);
	g_csgo.BoneAccessor = pattern::find(g_csgo.m_client_dll, XOR("8D 81 ? ? ? ? 50 8D 84 24")).add(2).to< size_t >();
	g_csgo.AnimOverlay = pattern::find(g_csgo.m_client_dll, XOR("8B 80 ? ? ? ? 8D 34 C8")).add(2).to< size_t >();
	g_csgo.SpawnTime = pattern::find(g_csgo.m_client_dll, XOR("F3 0F 5C 88 ? ? ? ? 0F")).add(4).to< size_t >();
	g_csgo.IsLocalPlayer = pattern::find(g_csgo.m_client_dll, XOR("74 ? 8A 83 ? ? ? ? 88")).add(4).to< size_t >();
	g_csgo.PlayerAnimState = pattern::find(g_csgo.m_client_dll, XOR("8B 8E ? ? ? ? 85 C9 74 3E")).add(2).to< size_t >();
	g_csgo.studioHdr = pattern::find(g_csgo.m_client_dll, XOR("8B 86 ? ? ? ? 89 44 24 10 85 C0")).add(2).to< size_t >();
	g_csgo.UTIL_TraceLine = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F0 83 EC 7C 56 52"));
	g_csgo.CTraceFilterSimple_vmt = g_csgo.UTIL_TraceLine.add(0x3D).to();
	g_csgo.CTraceFilterSkipTwoEntities_vmt = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? F3 0F 10 84 24 ? ? ? ? 8D 84 24 ? ? ? ? F3 0F 58 44 24")).rel32(1).add(0x59).to();
	g_csgo.LastBoneSetupTime = g_csgo.InvalidateBoneCache.add(0x11).to< size_t >();
	g_csgo.MostRecentModelBoneCounter = g_csgo.InvalidateBoneCache.add(0x1B).to< size_t >();

	// exported functions.
	g_csgo.RandomSeed = PE::GetExport(g_csgo.m_vstdlib_dll, HASH("RandomSeed")).as< CSGO::RandomSeed_t >();
	g_csgo.RandomInt = PE::GetExport(g_csgo.m_vstdlib_dll, HASH("RandomInt")).as< CSGO::RandomInt_t >();
	g_csgo.RandomFloat = PE::GetExport(g_csgo.m_vstdlib_dll, HASH("RandomFloat")).as< CSGO::RandomFloat_t >();

    g_netvars.init();
    g_entoffsets.init();
    g_netvars.SetupClassData();*/

	g_netvars.init();
	g_entoffsets.init();

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

class FamilyHookClient {
public:
	/*0x000*/ char pad01[0x15C];
	/*0x15C*/ Player* m_local;
	/*0x160*/ Weapon* m_weapon;
};

std::vector<Tab*> tabs;
std::vector<Element*> elements;
std::vector<Element*> AimbotElements;
Form* form = nullptr;
Button* saveButton = nullptr;
FamilyHookClient* g_familyCl = nullptr; // 1B B700
CVariables::RAGE* lastWeaponConfig = nullptr;

CVariables::RAGE* GetActiveRage(int id) {
	switch (g_familyCl->m_weapon->m_iItemDefinitionIndex()) {
	case WEAPON_GLOCK:
	case WEAPON_P2000:
	case WEAPON_USPS:
	case WEAPON_P250:
	case WEAPON_CZ75:
	case WEAPON_TEC9:
	case WEAPON_FIVESEVEN:
		if (!g_Vars.rage_pistols.override_default_config)
			break;

		return &g_Vars.rage_pistols;
	case WEAPON_REVOLVER:
		if (!g_Vars.rage_revolver.override_default_config)
			break;

		return &g_Vars.rage_revolver;
	case WEAPON_DEAGLE:
		if (!g_Vars.rage_deagle.override_default_config)
			break;

		return &g_Vars.rage_deagle;
	case WEAPON_AWP:
		if (!g_Vars.rage_awp.override_default_config)
			break;

		return &g_Vars.rage_awp;
	case WEAPON_SSG08:
		if (!g_Vars.rage_scout.override_default_config)
			break;

		return &g_Vars.rage_scout;
	case WEAPON_SCAR20:
	case WEAPON_G3SG1:
		if (!g_Vars.rage_autosnipers.override_default_config)
			break;

		return &g_Vars.rage_autosnipers;
	default:
		break;
	}

	return &g_Vars.rage_default;
}

/*class AimbotSettings {
public:
	bool aimbot_enable;
	std::vector<int> hitboxes;
	std::vector<int> multipoints;
	int multipoint_intensity;
	float pointscale;
	float aim_height;
	bool seperate_pointscale;
	float head_pointscale;
	float limb_pointscale;
	std::vector<int> body_flags;
	std::vector<int> bind_flags;
	int scan_intensity;
	float minimum_damage;
	float minimum_override1;
	float minimum_override2;
	float hitchance;
	float hitchance_override1;
	float hitchance_override2;
	bool mindamage_hitchance_override;
	float prefer_hitchance;
	float prefer_baim;
	float minimum_assessed_damage;
	std::vector<int> autostop;
	std::vector<int> standing_baim;
	std::vector<int> strict_standing_baim;
	std::vector<int> air_baim;
	float limit_air_pointscale;
	std::vector<int> strict_air_baim;
	std::vector<int> avoid_body_edges;
	float prefer_fakebody_overlap;
	std::vector<int> head_accuracy_mode;
	float prefer_fakehead_overlap;
	bool reduce_hitchance;
	float reduce_hitchance_value;
	float reduce_hitchance_delay;
	bool reduce_hitchance_damage_override;
	std::vector<int> autostop_flags;
	int passive_delay_on_peek;
	int force_delay_on_peek;
};*/

void GetDefaultRageConfig() {
	for (auto e : AimbotElements) {
		if (strstr(e->m_file_id.c_str(), "aimbot_enable")) {
			g_Vars.rage_default.aimbot_enable = reinterpret_cast<Checkbox*>(e)->enabled;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "hitbox"))
		//{	
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = rage->aimbot_enable;
		//	continue;
		//}

		if (strstr(e->m_file_id.c_str(), "strength_val4"))
		{
			g_Vars.rage_default.multipoint_intensity = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "scale_body"))
		{
			g_Vars.rage_default.pointscale = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_head_height"))
		{
			g_Vars.rage_default.maximum_aim_height = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_custom"))
		{
			g_Vars.rage_default.seperate_pointscale = reinterpret_cast<Checkbox*>(e)->enabled;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_head"))
		{
			g_Vars.rage_default.pointscale_head = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_limbs"))
		{
			g_Vars.rage_default.pointscale_limbs = reinterpret_cast<Slider*>(e)->value;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "mindamage"))
		{
			g_Vars.rage_default.minimum_damage = reinterpret_cast<Slider*>(e)->value;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "body_flags_new4"))
		//{	
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue;
		//}
		//if (strstr(e->m_file_id.c_str(), "damage_flags_new4"))
		//{	
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue;
		//}

		if (strstr(e->m_file_id.c_str(), "scan_intensity"))
		{
			g_Vars.rage_default.multipoint_intensity = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "hitchance"))
		{
			g_Vars.rage_default.minimum_hitchance = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "hitchance_ratio"))
		{
			g_Vars.rage_default.prefer_hitchance = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "prefer_baim"))
		{
			g_Vars.rage_default.prefer_baim = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "workdamage"))
		{
			g_Vars.rage_default.minimal_assessed_damage = reinterpret_cast<Slider*>(e)->value;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "aimbot_autostop"))
		//{
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue; 
		//}

		if (strstr(e->m_file_id.c_str(), "head_baim"))
		{
			g_Vars.rage_default.standing_baim = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "ground_strict"))
		{
			g_Vars.rage_default.strict_standing_baim = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "air_baim"))
		{
			g_Vars.rage_default.air_baim = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "air_limit"))
		{
			g_Vars.rage_default.limit_air_pointscale = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "air_nonstrict"))
		{
			g_Vars.rage_default.strict_air_baim = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "body_accuracy"))
		{
			g_Vars.rage_default.avoid_body_edges = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "body_safe"))
		{
			g_Vars.rage_default.prefer_fakebody_overlap = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "head_accuracy2"))
		{
			g_Vars.rage_default.head_accuracy_mode = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "head_safe"))
		{
			g_Vars.rage_default.prefer_fakehead_overlap = reinterpret_cast<Slider*>(e)->value;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "mindamagealt"))
		{
			g_Vars.rage_default.minimum_damage_override_primary = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "mindamagealt2"))
		{
			g_Vars.rage_default.minimum_damage_override_secondary = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "override_hitchances"))
		{
			g_Vars.rage_default.minimum_damage_override_hitchance = reinterpret_cast<Checkbox*>(e)->enabled;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "hitchancealt"))
		{
			g_Vars.rage_default.minimum_hitchance_override_primary = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "hitchancealt2"))
		{
			g_Vars.rage_default.minimum_hitchance_override_secondary = reinterpret_cast<Slider*>(e)->value;
			continue;
		}


		if (strstr(e->m_file_id.c_str(), "reduce_hitchance_longrange2"))
		{
			g_Vars.rage_default.reduce_hitchance = reinterpret_cast<Checkbox*>(e)->enabled;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "longrange_hc"))
		{
			g_Vars.rage_default.reduce_hitchance_value = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "longrange_time"))
		{
			g_Vars.rage_default.reduce_hitchance_delay = reinterpret_cast<Slider*>(e)->value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "longrange_activates_with_low_md"))
		{
			g_Vars.rage_default.reduce_hitchance_damage_override = reinterpret_cast<Checkbox*>(e)->enabled;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "simplestop_flags3"))
		//{
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue; 
		//}
		if (strstr(e->m_file_id.c_str(), "body_delay_new2"))
		{
			g_Vars.rage_default.passive_delay_on_peek = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "body_force"))
		{
			g_Vars.rage_default.force_delay_on_peek = reinterpret_cast<Dropdown*>(e)->selected_index;
			continue;
		}
	}
}

static void __fastcall DoWeaponConfigs(CVariables::RAGE* rage) {
	//if (rage == lastWeaponConfig)
	//	return;

	// really really bad, pls fix asap
	for (auto e : AimbotElements) {
		if (strstr(e->m_file_id.c_str(), "aimbot_enable")) {
			reinterpret_cast<Checkbox*>(e)->enabled = rage->aimbot_enable;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "hitbox"))
		//{	
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = rage->aimbot_enable;
		//	continue;
		//}

		if (strstr(e->m_file_id.c_str(), "strength_val4"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->multipoint_intensity;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "scale_body"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->pointscale;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_head_height"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->maximum_aim_height;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_custom"))
		{
			reinterpret_cast<Checkbox*>(e)->enabled = rage->seperate_pointscale;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_head"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->pointscale_head;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "scale_limbs"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->pointscale_limbs;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "mindamage"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimum_damage;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "body_flags_new4"))
		//{	
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue;
		//}
		//if (strstr(e->m_file_id.c_str(), "damage_flags_new4"))
		//{	
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue;
		//}

		if (strstr(e->m_file_id.c_str(), "scan_intensity"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->multipoint_intensity;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "hitchance"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimum_hitchance;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "hitchance_ratio"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->prefer_hitchance;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "prefer_baim"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->prefer_baim;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "workdamage"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimal_assessed_damage;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "aimbot_autostop"))
		//{
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue; 
		//}

		if (strstr(e->m_file_id.c_str(), "head_baim"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->standing_baim;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "ground_strict"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->strict_standing_baim;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "air_baim"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->air_baim;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "air_limit"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->limit_air_pointscale;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "air_nonstrict"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->strict_air_baim;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "body_accuracy"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->avoid_body_edges;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "body_safe"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->prefer_fakebody_overlap;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "head_accuracy2"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->head_accuracy_mode;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "head_safe"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->prefer_fakehead_overlap;
			continue;
		}

		if (strstr(e->m_file_id.c_str(), "mindamagealt"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimum_damage_override_primary;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "mindamagealt2"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimum_damage_override_secondary;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "override_hitchances"))
		{
			reinterpret_cast<Checkbox*>(e)->enabled = rage->minimum_damage_override_hitchance;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "hitchancealt"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimum_hitchance_override_primary;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "hitchancealt2"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->minimum_hitchance_override_secondary;
			continue;
		}


		if (strstr(e->m_file_id.c_str(), "reduce_hitchance_longrange2"))
		{
			reinterpret_cast<Checkbox*>(e)->enabled = rage->reduce_hitchance;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "longrange_hc"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->reduce_hitchance_value;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "longrange_time"))
		{
			reinterpret_cast<Slider*>(e)->value = rage->reduce_hitchance_delay;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "longrange_activates_with_low_md"))
		{
			reinterpret_cast<Checkbox*>(e)->enabled = rage->reduce_hitchance_damage_override;
			continue;
		}

		//if (strstr(e->m_file_id.c_str(), "simplestop_flags3"))
		//{
		//	reinterpret_cast<MultiDropdown*>(e)->enabled = g_Vars.rage_default.aimbot_enable;
		//	continue; 
		//}
		if (strstr(e->m_file_id.c_str(), "body_delay_new2"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->passive_delay_on_peek;
			continue;
		}
		if (strstr(e->m_file_id.c_str(), "body_force"))
		{
			reinterpret_cast<Dropdown*>(e)->selected_index = rage->force_delay_on_peek;
			continue;
		}
	}
	//reinterpret_cast<MultiDropdown*>(AimbotElements[0])->m_active_items = rage->aimbot_enable;

	lastWeaponConfig = rage;
}

void __fastcall Hooks::hkAimbotThink(void* ecx) {
    // do weapon configs here
    if (g_Vars.rage.weapon_configs) {
		if (!g_familyCl) {
			g_familyCl = reinterpret_cast<FamilyHookClient*>(familyhookBase + 0x1BB700);
			GetDefaultRageConfig();
		}

		if (g_familyCl && g_familyCl->m_weapon) {
			DoWeaponConfigs(GetActiveRage(g_familyCl->m_weapon->m_iItemDefinitionIndex()));
		}
        //g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());
        //g_cl.m_weapon = g_cl.m_local->GetActiveWeapon();
	}
	else {
		if (lastWeaponConfig != nullptr) {
			DoWeaponConfigs(&g_Vars.rage_default);
			lastWeaponConfig = nullptr;
		}
	}

    return oAimbotThink(ecx);
}

void* __fastcall Hooks::hkFormDraw(void* ecx) {
    if (!form)
        form = reinterpret_cast<Form*>(familyhookBase + 0x141ED0);

    g_gui.open = form->m_open;

    if (tabs.size() == 0) {
        if (form) {
            tabs = form->m_tabs.get();
            for (auto tab : tabs) {
                if (tab) {
					auto tabElements = tab->m_elements.get();
					for (int i = 0; i < tabElements.size(); i++) {
						auto e = tabElements[i];

                        elements.push_back(e);
						Log() << "Found element " << e->m_file_id.c_str() << " at " << std::format("{:X}", (DWORD)e);
                        if (tab == tabs[0]) {
                            auto file_id = e->m_file_id.c_str();

                            if (file_id == "accuracy" ||
                                file_id == "autofire")
                                continue;

                            AimbotElements.push_back(e);
                        }
                    }
                }
            }
            if (elements.size() != 0) {
                for (auto e : elements) {
                    /*if (e->m_label.c_str() == "save") {
                        saveButton = reinterpret_cast<Button*>(e);

                        if (MH_CreateHook(g_memory.Get(e, 2), &hkButtonClick, reinterpret_cast<void**>(&oButtonClick)))
                            throw std::runtime_error("Unable to hook Button::Click()");
						break;
                    }*/
                }
            }
        }
    }

    return oFormDraw(ecx);
}

void __fastcall Hooks::hkButtonClick(Button* ecx) {
    if (ecx == saveButton) {
        // restore our aimbot element values to the default config
    }

    return oButtonClick(ecx);
}

//const char* Hooks::hkConfigSave(void* a1, void* a2, void* a3, void* a4) {
//    return oConfigSave(a1, a2, a3, a4);
//}

void OnMapload() {
	// store class ids.
	g_netvars.SetupClassData();
}

void __fastcall Hooks::hkLevelInitPostEntity(void* ecx) {
	OnMapload();

	// invoke original method.
	return oLevelInitPostEntity(ecx);
}