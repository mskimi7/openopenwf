#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <Psapi.h>

#include <string>
#include <format>
#include <vector>

#include "game_data/warframe_string.h"
#include "game_data/object_type_names.h"
#include "game_data/resources.h"

#define MEMBER_OFFSET(type, offset) ((type)((char*)this + (offset)))

// In case we need to unload the DLL.
inline HINSTANCE g_hInstDll;

inline char* g_entryPointAddress;
inline DWORD g_entryPointOldProtect;

inline std::wstring g_wfExeDirectory;

std::wstring UTF8ToWide(const std::string& s);
std::string WideToUTF8(const std::wstring& s);

struct OpenWFConfig {
	std::string serverHost = "127.0.0.1";
	bool disableNRS = true;

	void PrintToConsole();
};

inline OpenWFConfig g_Config;

struct AssetDownloader {
	inline WarframeString* GetCacheManifestHash() { return MEMBER_OFFSET(WarframeString*, 0x1F0); }

	static inline AssetDownloader* Instance;
};

inline const char* g_BuildLabelStringPtr;

std::vector<unsigned char*> SignatureScan(const char* pattern, const char* mask, unsigned char* data, size_t length);
std::string AESDecrypt(const std::string& inputData, const std::string& key, const std::string& iv);

void LoadConfig();
void PlaceHooks();
std::string OWFGetBuildLabel();

void OpenWFLog(const std::string& message);
#define OWFLog(fmt, ...) OpenWFLog(std::format(fmt, __VA_ARGS__))

__declspec(noreturn) void OpenWFFatalExit(const std::string& reason, const std::string& func, const std::string& file, int line);
#define FATAL_EXIT(s) OpenWFFatalExit(s, __FUNCTION__, __FILE__, __LINE__)

inline void (*InitStringFromBytes)(WarframeString*, const char*);
inline void (*WFFree)(void*);
inline TypeMgr* (*GetTypeMgr)();
inline void (*GetPropertyText)(TypeMgr*, ObjectType*, WarframeString*, int);
