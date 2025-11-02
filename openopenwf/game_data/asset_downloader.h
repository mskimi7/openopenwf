#pragma once

#include <vector>
#include <utility>
#include <Windows.h>

#include "warframe_string.h"
#include "object_type_names.h"

#ifndef MEMBER_OFFSET
#define MEMBER_OFFSET(type, offset) ((type)((char*)this + (offset)))

struct AssetDownloader {
	inline CRITICAL_SECTION* GetManifestTreeLock() { return **MEMBER_OFFSET(CRITICAL_SECTION***, 0x1C8); }
	inline void* GetManifestTree() { return MEMBER_OFFSET(void*, 0x1D0); }
	inline WarframeString* GetCacheManifestHash() { return MEMBER_OFFSET(WarframeString*, 0x1F0); }

	std::unique_ptr<std::vector<std::string>> GetAllTypes();
	static inline AssetDownloader* Instance;
};

#endif
