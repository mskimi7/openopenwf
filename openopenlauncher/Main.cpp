#include "openwf.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	wchar_t processName[MAX_PATH];
	DWORD processNameSize = std::size(processName);
	if (QueryFullProcessImageNameW(GetCurrentProcess(), 0, processName, &processNameSize))
	{
		wchar_t* fileName = PathFindFileNameW(processName);
		g_configFilePath = std::wstring(processName, fileName - processName) + L"oowf_launch.cfg";
	}

	CreateLaunchDialog();
	return 0;
}
