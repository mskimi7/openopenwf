#pragma once

#include "common.h"

#include <unordered_set>

struct TypeMgrEntry {
	int nameIndex;
	int pad;
	ObjectType* type;
};

struct TypeMgrDirEntry {
	WarframeVector<TypeMgrEntry>* GetTypes() { return MEMBER_OFFSET(WarframeVector<TypeMgrEntry>*, 0x18); }
};

// list of types in a single "directory"
struct TypeMgrDirTypeList {
	int pathIndex;
	int pad;
	TypeMgrDirEntry* entry;
};

struct TypeMgr {
	WarframeVector<TypeMgrDirTypeList>* GetAllTypePaths() { return MEMBER_OFFSET(WarframeVector<TypeMgrDirTypeList>*, 0xE0); }

	std::unique_ptr<std::unordered_set<std::string>> GetRegisteredTypes();

	static TypeMgr* GetInstance();
};

inline TypeMgr* (*GetTypeMgr)();
inline void (*GetPropertyText)(TypeMgr*, ObjectType*, WarframeString*, int);
