#include "includes.h"
#include "logging.h"

DWORD WINAPI Setup( LPVOID instance ) {
	try {
		gLogger.attach( "family-addon" );

		Log( ) << "g_vars.init( );";
		g_Vars.Create( );

		Log( ) << "g_memory.init( );";
		g_memory.init( );

		Log( ) << "g_gui.init( );";
		g_gui.init( ); // these can be initialized first as Client::init( ) is very slow

		//g_hooks.init( );
		//g_Vars.Create( );

		Log( ) << "Client::init( instance );";
		Client::init( instance );
	}
	catch ( const std::exception& e ) {
		Log( ERR ) << "setup error";
		MessageBeep( MB_ICONERROR );
		MessageBoxA( nullptr, e.what( ), "setup error", MB_OK | MB_ICONEXCLAMATION );
	}

	while ( !GetAsyncKeyState( VK_END ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

	g_hooks.destroy( );
	g_gui.Destroy( );
	gLogger.detach( );
	FreeLibraryAndExitThread( static_cast< HMODULE >( instance ), EXIT_SUCCESS );
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID lpReserved )
{
	if ( dwReason == DLL_PROCESS_ATTACH ) {
		DisableThreadLibraryCalls( hModule );

		const HANDLE thread = CreateThread( nullptr, NULL, Setup, hModule, NULL, nullptr );

		if ( thread )
			CloseHandle( thread );


	}

	return TRUE;
}

