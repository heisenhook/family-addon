#include "includes.h"
#include "logging.h"

DWORD WINAPI Setup( LPVOID instance ) {
	try {
		gLogger.attach("family-addon");
		g_memory.init( );
		g_gui.init( );
		g_hooks.init( );
		g_Vars.Create( );
	}
	catch ( const std::exception& error ) {
		MessageBeep( MB_ICONERROR );
		MessageBoxA( 0, error.what( ), "yeah", MB_OK | MB_ICONEXCLAMATION );
		goto UNLOAD;
	}

	while ( !GetAsyncKeyState( VK_END ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

UNLOAD :
	gLogger.detach();
	g_hooks.destroy( );
	g_gui.Destroy( );
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

