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

public:
	// forward declarations
	class IRecipientFilter;

	// prototypes.
	using PaintTraverse_t = void( __thiscall* )( void*, VPANEL, bool, bool );
	using DoPostScreenSpaceEffects_t = bool( __thiscall* )( void*, CViewSetup* );
	using CreateMove_t = bool( __thiscall* )( void*, float, CUserCmd* );
	using LevelInitPostEntity_t = void( __thiscall* )( void* );
	using LevelShutdown_t = void( __thiscall* )( void* );
	using LevelInitPreEntity_t = void( __thiscall* )( void*, const char* );
	using IN_KeyEvent_t = int( __thiscall* )( void*, int, int, const char* );
	using FrameStageNotify_t = void( __thiscall* )( void*, Stage_t );
	using UpdateClientSideAnimation_t = void( __thiscall* )( void* );
	using GetActiveWeapon_t = Weapon * ( __thiscall* )( void* );
	using DoExtraBoneProcessing_t = void( __thiscall* )( void*, int, int, int, int, int, int );
	using BuildTransformations_t = void( __thiscall* )( void*, int, int, int, int, int, int );
	using CalcViewModelView_t = void( __thiscall* )( void*, vec3_t&, ang_t& );
	using InPrediction_t = bool( __thiscall* )( void* );
	using OverrideView_t = void( __thiscall* )( void*, CViewSetup* );
	using LockCursor_t = void( __thiscall* )( void* );
	using RunCommand_t = void( __thiscall* )( void*, Entity*, CUserCmd*, IMoveHelper* );
	using ProcessPacket_t = void( __thiscall* )( void*, void*, bool );
	using SendDatagram_t = int( __thiscall* )( void*, void* );
	// using CanPacket_t                = bool( __thiscall* )( void* );
	using PlaySound_t = void( __thiscall* )( void*, const char* );
	using GetScreenSize_t = void( __thiscall* )( void*, int&, int& );
	using Push3DView_t = void( __thiscall* )( void*, CViewSetup&, int, void*, void* );
	using SceneEnd_t = void( __thiscall* )( void* );
	using DrawModelExecute_t = void( __thiscall* )( void*, uintptr_t, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t* );
	using ComputeShadowDepthTextures_t = void( __thiscall* )( void*, const CViewSetup&, bool );
	using GetInt_t = int( __thiscall* )( void* );
	using GetBool_t = bool( __thiscall* )( void* );
	using IsConnected_t = bool( __thiscall* )( void* );
	using IsHLTV_t = bool( __thiscall* )( void* );
	using OnEntityCreated_t = void( __thiscall* )( void*, Entity* );
	using OnEntityDeleted_t = void( __thiscall* )( void*, Entity* );
	using RenderSmokeOverlay_t = void( __thiscall* )( void*, bool );
	using ShouldDrawFog_t = bool( __thiscall* )( void* );
	using ShouldDrawParticles_t = bool( __thiscall* )( void* );
	using Render2DEffectsPostHUD_t = void( __thiscall* )( void*, const CViewSetup& );
	using OnRenderStart_t = void( __thiscall* )( void* );
	using RenderView_t = void( __thiscall* )( void*, const CViewSetup&, const CViewSetup&, int, int );
	using GetMatchSession_t = CMatchSessionOnlineHost * ( __thiscall* )( void* );
	using OnScreenSizeChanged_t = void( __thiscall* )( void*, int, int );
	using OverrideConfig_t = bool( __thiscall* )( void*, MaterialSystem_Config_t*, bool );
	using PostDataUpdate_t = void( __thiscall* )( void*, DataUpdateType_t );
	using TempEntities_t = bool( __thiscall* )( void*, void* );
	using EmitSound_t = void( __thiscall* )( void*, IRecipientFilter&, int, int, const char*, unsigned int, const char*, float, float, int, int, int, const vec3_t*, const vec3_t*, void*, bool, float, int );
	// using PreDataUpdate_t            = void( __thiscall* )( void*, DataUpdateType_t );

public:

    // hooks
    using EndSceneFn = long( __thiscall* )( void*, IDirect3DDevice9* ) noexcept;
    inline static EndSceneFn EndSceneOriginal = nullptr;
    static long __stdcall EndScene( IDirect3DDevice9* device ) noexcept;

    using ResetFn = HRESULT( __thiscall* )( void*, IDirect3DDevice9*, D3DPRESENT_PARAMETERS* ) noexcept;
    inline static ResetFn ResetOriginal = nullptr;
    static HRESULT __stdcall Reset( IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params ) noexcept;

	using Aimbot__ThinkFn = void( __thiscall* )( void* );
	inline static Aimbot__ThinkFn oAimbotThink = nullptr;
	static void __fastcall hkAimbotThink(void* ecx);

	using Form__DrawFn = void*( __thiscall* )( void* );
	inline static Form__DrawFn oFormDraw = nullptr;
	static void* __fastcall hkFormDraw(void* ecx);

	//using Config__SaveFn = const char*( __stdcall* )( void*, void*, void*, void*);
	//inline static Config__SaveFn oConfigSave = nullptr;
	//static const char* hkConfigSave(void* a1, void* a2, void* a3, void* a4);

    HANDLE watcherHandle = nullptr;

public:
	// vmts.
	VMT m_panel;
	VMT m_client_mode;
	VMT m_client;
	VMT m_client_state;
	VMT m_engine;
	VMT m_engine_sound;
	VMT m_prediction;
	VMT m_surface;
	VMT m_render;
	VMT m_net_channel;
	VMT m_render_view;
	VMT m_model_render;
	VMT m_shadow_mgr;
	VMT m_view_render;
	VMT m_match_framework;
	VMT m_material_system;
	VMT m_fire_bullets;
	VMT m_net_show_fragments;

	// player shit.
	std::array< VMT, 64 > m_player;

	// cvars
	VMT m_debug_spread;

	// wndproc old ptr.
	WNDPROC m_old_wndproc;

	// old player create fn.
	//DoExtraBoneProcessing_t     m_DoExtraBoneProcessing;
	//UpdateClientSideAnimation_t m_UpdateClientSideAnimation;
	//GetActiveWeapon_t           m_GetActiveWeapon;
	//BuildTransformations_t      m_BuildTransformations;

	// netvar proxies.
	RecvVarProxy_t m_Pitch_original;
	RecvVarProxy_t m_Body_original;
	RecvVarProxy_t m_Force_original;
	RecvVarProxy_t m_AbsYaw_original;
};

extern Hooks g_hooks;
