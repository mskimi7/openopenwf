#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <Psapi.h>

#include <string>
#include <format>
#include <vector>

#include "game_data/common.h"
#include "game_data/asset_downloader.h"
#include "game_data/resources.h"
#include "game_data/type_mgr.h"
#include "clr/clr.h"

#define REDIRECTOR_NAME "openopenwf_2"

using namespace std::string_literals;

// In case we need to unload the DLL.
inline HINSTANCE g_hInstDll;

inline char* g_entryPointAddress;
inline DWORD g_entryPointOldProtect;

inline std::wstring g_wfExeDirectory;
inline bool g_DisableWin32NotifyMessages;

std::wstring UTF8ToWide(const std::string& s);
std::string WideToUTF8(const std::wstring& s);

struct OpenWFConfig {
	std::string serverHost = "127.0.0.1";
	bool disableNRS = true;
	int httpPort = 80;
	int httpsPort = 443;
	bool disableCLR = false;

	void PrintToConsole();
};

inline OpenWFConfig g_Config;

inline const char* g_BuildLabelStringPtr;

std::vector<unsigned char*> SignatureScan(const char* pattern, const char* mask, unsigned char* data, size_t length);
std::string Base64Encode(const std::string& inputData);
std::string AESDecrypt(const std::string& inputData, const std::string& key, const std::string& iv);

void LoadConfig();
void InitCLR();
void PlaceHooks();
std::string OWFGetBuildLabel();

void OpenWFLog(const std::string& message);
void OpenWFLogColor(const std::string& message, unsigned short color);
#define OWFLog(fmt, ...) OpenWFLog(std::format(fmt, __VA_ARGS__))
#define OWFLogColor(color, fmt, ...) OpenWFLogColor(std::format(fmt, __VA_ARGS__), color)

__declspec(noreturn) void OpenWFFatalExit(const std::string& reason, const std::string& func, const std::string& file, int line);
#define FATAL_EXIT(s) OpenWFFatalExit(s, __FUNCTION__, __FILE__, __LINE__)

inline void (*InitStringFromBytes)(WarframeString*, const char*);
inline void (*WFFree)(void*);

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
