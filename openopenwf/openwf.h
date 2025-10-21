#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <Psapi.h>

#include <string>
#include <format>
#include <vector>

// In case we need to unload the DLL.
inline HINSTANCE g_hInstDll;

inline char* g_entryPointAddress;
inline DWORD g_entryPointOldProtect;

inline std::wstring g_wfExeDirectory;

std::vector<unsigned char*> SignatureScan(const char* pattern, const char* mask, unsigned char* data, size_t length);

std::wstring UTF8ToWide(const std::string& s);
std::string WideToUTF8(const std::wstring& s);

struct OpenWFConfig {
	std::string serverHost = "127.0.0.1";

	void PrintToConsole();
};

inline OpenWFConfig g_Config;

struct WarframeString {
	unsigned char buf[16] = { 0 };

	inline WarframeString()
	{
		buf[15] = 0x0F;
	}

	inline char* GetPtr()
	{
		if (buf[15] == 0xFF)
			return *(char**)buf;

		return (char*)buf;
	}

	inline size_t GetSize() const
	{
		if (buf[15] == 0xFF)
			return *(unsigned int*)(buf + 8) & 0xFFFFFFF;

		return 15 - buf[15];
	}

	inline std::string GetText() const
	{
		const char* strbuf = (const char*)buf;
		int size;

		if (buf[15] == 0xFF)
		{
			strbuf = *(const char**)strbuf;
			size = *(int*)(buf + 8) & 0xFFFFFFF;
		}
		else
		{
			size = 15 - buf[15];
		}

		return std::string(strbuf, size);
	}
};

struct AssetDownloader {
	inline WarframeString* GetCacheManifestHash() { return (WarframeString*)((char*)this + 0x1F0); }
};

inline AssetDownloader** g_AssetDownloaderPtr;
inline const char* g_BuildLabelStringPtr;

inline std::string OWFGetBuildLabel()
{
	std::string fullBuildLabel = g_BuildLabelStringPtr;
	size_t spaceIndex = fullBuildLabel.find(' ');
	if (spaceIndex == std::string::npos)
		return fullBuildLabel;

	return fullBuildLabel.substr(0, spaceIndex);
}

void LoadConfig();
void PlaceHooks();

std::string AESDecrypt(const std::string& inputData, const std::string& key, const std::string& iv);

void OpenWFLog(const std::string& message);
#define OWFLog(fmt, ...) OpenWFLog(std::format(fmt, __VA_ARGS__))

__declspec(noreturn) void OpenWFFatalExit(const std::string& reason, const std::string& func, const std::string& file, int line);
#define FATAL_EXIT(s) OpenWFFatalExit(s, __FUNCTION__, __FILE__, __LINE__)
