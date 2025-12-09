#include "includes.h"

Client g_cl{ };

// init routine.
ulong_t __stdcall Client::init( void* arg ) {
	// if not in interwebz mode, the driver will not set the username.
	g_cl.m_user = XOR( "user" );

	// stop here if we failed to acquire all the data needed from csgo.
	if ( !g_csgo.init( ) )
		return 0;

	// welcome the user.
	//g_notify.add( tfm::format( XOR( "welcome %s\n" ), g_cl.m_user ) );

	return 1;
}

void Client::OnMapload( ) {
	// store class ids.
	g_netvars.SetupClassData( );

	// createmove will not have been invoked yet.
	// but at this stage entites have been created.
	// so now we can retrive the pointer to the local player.
	m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// world materials.
	//Visuals::ModulateWorld( );

	// init knife shit.
	//g_skins.load( );

	m_sequences.clear( );

	// if the INetChannelInfo pointer has changed, store it for later.
	g_csgo.m_net = g_csgo.m_engine->GetNetChannelInfo( );

	//if ( g_csgo.m_net ) {
	//	g_hooks.m_net_channel.reset( );
	//	g_hooks.m_net_channel.init( g_csgo.m_net );
	//	g_hooks.m_net_channel.add( INetChannel::PROCESSPACKET, util::force_cast( &Hooks::ProcessPacket ) );
	//	g_hooks.m_net_channel.add( INetChannel::SENDDATAGRAM, util::force_cast( &Hooks::SendDatagram ) );
	//}
}