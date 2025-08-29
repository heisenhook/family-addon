#include "address.h"
#include <cstring>
#include <iostream>

uintptr_t GetModuleBaseByName( const wchar_t* moduleName ) {
    // If moduleName == nullptr -> return base of EXE
    if ( !moduleName ) {
        return reinterpret_cast< uintptr_t >( GetModuleHandleW( NULL ) );
    }
    HMODULE h = GetModuleHandleW( moduleName );
    return reinterpret_cast< uintptr_t >( h );
}

uintptr_t ResolveAddressFromIDA( uintptr_t idaVA, uintptr_t idaImageBase, const wchar_t* moduleName ) {
    uintptr_t rva = idaVA - idaImageBase;
    uintptr_t moduleBase = GetModuleBaseByName( moduleName );
    if ( !moduleBase ) return 0;
    return moduleBase + rva;
}

bool VerifyBytes( uintptr_t runtimeAddr, const unsigned char* idaBytes, size_t length ) {
    if ( !runtimeAddr || !idaBytes || length == 0 ) return false;

    // Ensure readable
    MEMORY_BASIC_INFORMATION mbi;
    if ( !VirtualQuery( reinterpret_cast< LPCVOID >( runtimeAddr ), &mbi, sizeof( mbi ) ) )
        return false;
    if ( !( mbi.Protect & ( PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE ) ) )
        ; // still attempt; some pages may be RWX; we proceed carefully

    __try {
        for ( size_t i = 0; i < length; ++i ) {
            unsigned char memByte = *reinterpret_cast< unsigned char* >( runtimeAddr + i );
            // IDA may display 'call rel32' with relative value; but comparing first few fixed bytes is useful
            if ( idaBytes[ i ] != memByte )
                return false;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {
        return false;
    }
    return true;
}

static uintptr_t GetModuleSize( HMODULE hModule ) {
    if ( !hModule ) return 0;
    PIMAGE_DOS_HEADER dos = ( PIMAGE_DOS_HEADER )hModule;
    PIMAGE_NT_HEADERS nt = ( PIMAGE_NT_HEADERS )( ( uintptr_t )hModule + dos->e_lfanew );
    return nt->OptionalHeader.SizeOfImage;
}

uintptr_t PatternScanModule( const wchar_t* moduleName, const unsigned char* pattern, const char* mask ) {
    HMODULE hModule = moduleName ? GetModuleHandleW( moduleName ) : GetModuleHandleW( NULL );
    if ( !hModule ) return 0;
    uintptr_t base = reinterpret_cast< uintptr_t >( hModule );
    uintptr_t size = GetModuleSize( hModule );
    const char* m = mask;
    size_t patLen = strlen( mask );

    for ( uintptr_t i = base; i < base + size - patLen; ++i ) {
        bool ok = true;
        for ( size_t j = 0; j < patLen; ++j ) {
            if ( m[ j ] != '?' && pattern[ j ] != *reinterpret_cast< unsigned char* >( i + j ) ) {
                ok = false;
                break;
            }
        }
        if ( ok ) return i;
    }
    return 0;
}
