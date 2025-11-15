#include "openwf.h"

// Tests if a Warframe path is valid. Argument: fully qualified path to Warframe.x64.exe.
// A path is valid if:
//   1. Warframe.x64.exe exists
//   2. Cache.Windows exists in the same directory, and it contains at least 10 .toc files
static bool IsValidWarframePath(const std::wstring& warframeExecutablePath)
{
	if (!FileExists(warframeExecutablePath.c_str()))
		return false;

	wchar_t pathBuffer[512];
	wcscpy(pathBuffer, warframeExecutablePath.c_str());

	LPWSTR fileName = PathFindFileNameW(pathBuffer);
	if (fileName == nullptr || fileName == pathBuffer)
		return false;

	wcscpy(fileName, L"Cache.Windows");
	if (!DirectoryExists(pathBuffer))
		return false;

	wcscat(fileName, L"\\*.toc");

	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(pathBuffer, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	int tocFilesFound = 0;

	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			++tocFilesFound;

		if (tocFilesFound >= 10)
			break;

	} while (FindNextFileW(hFind, &ffd) != 0);

	FindClose(hFind);
	return tocFilesFound >= 10;
}

std::wstring GuessWarframeSettings(std::optional<std::wstring>& language, std::optional<bool>& isDx11, const std::wstring& savedWarframePath)
{
	wchar_t regValue[512];
	DWORD regValueSize = sizeof(regValue);

	if (IsValidWarframePath(savedWarframePath))
		return savedWarframePath;

	if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Digital Extremes\\Warframe\\Launcher", L"DownloadDir", RRF_RT_REG_SZ, nullptr, regValue, &regValueSize) == ERROR_SUCCESS)
	{
		std::wstring regValueStr = regValue;
		regValueStr += L"\\Public\\Warframe.x64.exe";

		if (IsValidWarframePath(regValueStr))
		{
			regValueSize = sizeof(regValue);
			if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Digital Extremes\\Warframe\\Launcher", L"Language", RRF_RT_REG_SZ, nullptr, regValue, &regValueSize) == ERROR_SUCCESS)
				language = regValue;

			DWORD graphicsSetting = 0;
			regValueSize = sizeof(graphicsSetting);
			if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Digital Extremes\\Warframe\\Launcher", L"GraphicsAPI", RRF_RT_REG_DWORD, nullptr, &graphicsSetting, &regValueSize) == ERROR_SUCCESS)
				isDx11 = graphicsSetting == 0;

			return regValueStr;
		}
	}

	return L"";
}
