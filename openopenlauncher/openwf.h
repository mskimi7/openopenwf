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

void CreateLaunchDialog();
std::wstring GuessWarframeSettings(std::optional<std::wstring>& language, std::optional<bool>& isDx11);
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
