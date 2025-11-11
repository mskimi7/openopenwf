#pragma once

#include <vector>
#include <utility>
#include <unordered_set>
#include <Windows.h>

#include "common.h"

struct AssetDownloader {
	inline CRITICAL_SECTION* GetManifestTreeLock() { return **MEMBER_OFFSET(CRITICAL_SECTION***, 0x1C8); }
	inline void* GetManifestTree() { return MEMBER_OFFSET(void*, 0x1D0); }
	inline WarframeString* GetCacheManifestHash() { return MEMBER_OFFSET(WarframeString*, 0x1F0); }

	std::unique_ptr<std::vector<CompressedTypeName>> GetManifestTypes();
	static inline AssetDownloader* Instance;
};
