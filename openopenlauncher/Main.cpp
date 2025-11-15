#include "openwf.h"

#include <Shlwapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// set the current directory to launcher's executable so that we can read the config file easily
	wchar_t processName[MAX_PATH];
	DWORD processNameSize = std::size(processName);
	if (QueryFullProcessImageNameW(GetCurrentProcess(), 0, processName, &processNameSize))
	{
		PathRemoveFileSpecW(processName);
		SetCurrentDirectoryW(processName);
	}

	CreateLaunchDialog();
	return 0;
}
