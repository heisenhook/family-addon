#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <vector>

// Get base address of a loaded module by name or NULL (exe) if moduleName == nullptr
uintptr_t GetModuleBaseByName( const wchar_t* moduleName );

// Convert an IDA virtual address to runtime address:
// runtime = moduleBase + (idaVA - idaImageBase)
uintptr_t ResolveAddressFromIDA( uintptr_t idaVA, uintptr_t idaImageBase, const wchar_t* moduleName );

// Read and compare bytes at resolved address with 'pattern' from IDA
bool VerifyBytes( uintptr_t runtimeAddr, const unsigned char* idaBytes, size_t length );

// Simple pattern scan within a module (useful if RVA conversion fails)
uintptr_t PatternScanModule( const wchar_t* moduleName, const unsigned char* pattern, const char* mask );
