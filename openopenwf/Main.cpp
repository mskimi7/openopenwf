#include "openwf.h"

#include <shlwapi.h>
#include <intrin.h>

// mov rax, <address of entryPointTrampoline>
// jmp rax
static unsigned char entryPointOverwriteBytes[] = { 0x48, 0xB8, 0xDD, 0xDD, 0xCC, 0xCC, 0xBB, 0xBB, 0xAA, 0xAA, 0xFF, 0xE0 };

// sub rsp, 0x28; shadow stack + 8 bytes for stack alignment fixup
// mov rax, <address of HookedEntryPoint>
// call rax
// add rsp, 0x28
// mov rax, <address of original entry point>
// jmp rax
static unsigned char entryPointTrampoline[] = { 0x48, 0x83, 0xEC, 0x28, 0x48, 0xB8, 0xDD, 0xCC, 0xBB, 0xAA, 0xDD, 0xCC, 0xBB, 0xAA, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x28, 0x48, 0xB8, 0xDD, 0xCC, 0xBB, 0xAA, 0xDD, 0xCC, 0xBB, 0xAA, 0xFF, 0xE0 };

static unsigned char entryPointOriginalBytes[sizeof(entryPointOverwriteBytes)];

static void HookedEntryPoint()
{
	OWFLogColor(15, "================================================================================\n");
	OWFLogColor(15, "=====                          OpenWF Enabler (v4)                         =====\n");
	OWFLogColor(15, "=====                                                                      =====\n");
	OWFLogColor(15, "====="); OWFLogColor(10, "          >>> Press CTRL+P to (re)open OpenWF Inspector <<<           "); OWFLogColor(15, "=====\n");
	OWFLogColor(15, "================================================================================\n\n");

	LoadConfig();
	g_Config.PrintToConsole();

	InitCLR();
	PlaceHooks();

	// restore original entry point and continue execution from there
	memcpy(g_entryPointAddress, entryPointOriginalBytes, sizeof(entryPointOriginalBytes));
	VirtualProtect(g_entryPointAddress, sizeof(entryPointOriginalBytes), g_entryPointOldProtect, &g_entryPointOldProtect);
}

static void HookEntryPoint()
{
	DWORD unused;

	PIMAGE_DOS_HEADER hdr = (PIMAGE_DOS_HEADER)GetModuleHandleA(nullptr);
	PIMAGE_NT_HEADERS64 nt = (PIMAGE_NT_HEADERS64)((char*)hdr + hdr->e_lfanew);

	g_entryPointAddress = (char*)hdr + nt->OptionalHeader.AddressOfEntryPoint;

	*(PULONG_PTR)(entryPointOverwriteBytes + 2) = (ULONG_PTR)entryPointTrampoline;
	*(PULONG_PTR)(entryPointTrampoline + 6) = (ULONG_PTR)(void*)&HookedEntryPoint;
	*(PULONG_PTR)(entryPointTrampoline + 0x16) = (ULONG_PTR)g_entryPointAddress;
	VirtualProtect(g_entryPointAddress, sizeof(entryPointOverwriteBytes), PAGE_EXECUTE_READWRITE, &g_entryPointOldProtect);
	VirtualProtect(entryPointTrampoline, sizeof(entryPointTrampoline), PAGE_EXECUTE_READWRITE, &unused);

	memcpy(entryPointOriginalBytes, g_entryPointAddress, sizeof(entryPointOriginalBytes));
	memcpy(g_entryPointAddress, entryPointOverwriteBytes, sizeof(entryPointOverwriteBytes));
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		BOOL allocConsoleResult = AllocConsole();

		g_hInstDll = hinstDLL;

		// check if we injected to Warframe.x64.exe and not some other process
		wchar_t processName[MAX_PATH];
		DWORD processNameSize = std::size(processName);
		if (QueryFullProcessImageNameW(GetCurrentProcess(), 0, processName, &processNameSize))
		{
			wchar_t* fileName = PathFindFileNameW(processName);
			g_wfExeDirectory = std::wstring(processName, fileName - processName);

			if (_wcsicmp(fileName, L"warframe.x64.exe") != 0)
			{
				std::wstring msg = std::format(L"You have possibly injected OpenWF Enabler into the wrong process (expected Warframe.x64.exe but found {})\n\nContinue injection? Choose \"No\" if unsure.", fileName);
				if (MessageBoxW(nullptr, msg.c_str(), L"OpenWF Enabler - Executable name mismatch", MB_ICONWARNING | MB_YESNOCANCEL) != IDYES)
				{
					if (allocConsoleResult)
						FreeConsole();

					return FALSE;
				}
			}
		}

		HookEntryPoint(); // injector will wait until DllMain executes to completion & initialization will continue inside HookedEntryPoint
	}

	return TRUE;
}
