#include "../openwf.h"

#include <Shlwapi.h>
#include <MetaHost.h>
#pragma comment(lib, "mscoree.lib")

static unsigned char* GetNativeEvent()
{
	return nullptr;
}

static void FreeNativeMemory(unsigned char* ptr)
{
	delete ptr;
}

static std::wstring BuildInitArgument()
{
	return std::to_wstring((ULONG_PTR)GetNativeEvent) + L',' + std::to_wstring((ULONG_PTR)FreeNativeMemory);
}

void InitCLR()
{
	wchar_t dllPath[MAX_PATH];
	if (!GetModuleFileNameW(g_hInstDll, dllPath, std::size(dllPath)))
		FATAL_EXIT(std::format("Could not obtain name of the openopenwf DLL: code {}", GetLastError()));

	wcscpy(PathFindFileNameW(dllPath), L"openopenclr.dll");

	ICLRMetaHost* clrMetaHost = nullptr;
	HRESULT hResult = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&clrMetaHost));
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("CLRCreateInstance failed: code {}", hResult));

	ICLRRuntimeInfo* clrRuntimeInfo = nullptr;
	hResult = clrMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&clrRuntimeInfo)); // this .NET should be installed by default on all Windows machines
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format(".NET GetRuntime v4.0.30319 failed: code {}", hResult));

	BOOL isLoadable = FALSE;
	if (!SUCCEEDED(clrRuntimeInfo->IsLoadable(&isLoadable)) || !isLoadable)
		FATAL_EXIT(".NET Runtime v4.0.30319 cannot be loaded!");

	ICLRRuntimeHost* clrRuntimeHost = nullptr;
	hResult = clrRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&clrRuntimeHost));
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("GetInterface for CLRRuntimeHost failed: code {}", hResult));

	hResult = clrRuntimeHost->Start();
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("CLRRuntimeHost could not be started: code {}", hResult));

	DWORD unused;
	hResult = clrRuntimeHost->ExecuteInDefaultAppDomain(dllPath, L"openopenclr.Program", L"Main", BuildInitArgument().c_str(), &unused);
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("Cannot load manager library openopenclr.dll: code {}", hResult));

	OWFLog("CLR successfully initialized"); // horror!
}
