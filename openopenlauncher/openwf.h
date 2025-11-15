#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <string>
#include <format>
#include <optional>

struct LaunchSettings {
	std::wstring warframeExePath;
	std::wstring langCode; // e.g. "en" or "zh"
	bool isDx11 = true;
};

void SaveLaunchSettings(const std::wstring& warframeExePath, const std::wstring& langCode, bool isDx11);
std::optional<LaunchSettings> LoadLaunchSettings();

void CreateLaunchDialog();
std::wstring GuessWarframeSettings(std::optional<std::wstring>& language, std::optional<bool>& isDx11, const std::wstring& savedWarframePath);
bool LaunchWarframe(HWND mainWindow, const std::wstring& wfExePath, const std::wstring& dllPath, const std::wstring& commandLineArgs);

inline BOOL FileExists(LPCWSTR szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

inline BOOL DirectoryExists(LPCWSTR szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

inline std::wstring UTF8ToWide(const std::string& s)
{
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	if (wideLen == 0)
		return L"";

	std::wstring wide(wideLen, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &wide[0], wideLen);

	return wide;
}

inline std::string WideToUTF8(const std::wstring& s)
{
	int mbLen = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
	if (mbLen == 0)
		return "";

	std::string multibyte(mbLen, 0);
	WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &multibyte[0], mbLen, nullptr, nullptr);

	return multibyte;
}
