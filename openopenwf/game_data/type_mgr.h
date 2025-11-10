#pragma once

#include "common.h"

#include <unordered_set>

enum TypeState: unsigned short {
	TypeState_Broken = 0x400
};

struct TypeMgrEntry;
struct TypeMgrDirEntry;
struct TypeMgrDirTypeList;

struct TypeMgrDirEntry {
	int pathIndex;
	int unk1;
	int unk2;
	int unk3;
	int unk4;
	int unk5;
	WarframeVector<TypeMgrEntry> types;
};

// list of types in a single "directory"
struct TypeMgrDirTypeList {
	int pathIndex;
	int pad;
	TypeMgrDirEntry* entry;
};

struct ObjectType {
	void* vtable;
	int unk0;
	int unk1;
	TypeMgrDirEntry* directory;
	ObjectType* parent;
	int typeId;
	int unk3;
	TypeState stateFlags;
	int nameIndex;

	CompressedTypeName GetName() const
	{
		CompressedTypeName n;
		n.pathIndex = directory ? directory->pathIndex : 0;
		n.nameIndex = nameIndex;
		return n;
	}
};

struct TypeMgrEntry {
	int nameIndex;
	int pad;
	ObjectType* type;
};

struct TypeMgr {
	WarframeVector<TypeMgrDirTypeList>* GetAllTypePaths() { return MEMBER_OFFSET(WarframeVector<TypeMgrDirTypeList>*, 0xE0); }

	std::unique_ptr<std::vector<CompressedTypeName>> GetRegisteredTypes();

	static TypeMgr* GetInstance();
};

inline TypeMgr* (*GetTypeMgr)();
inline void (*GetPropertyText)(TypeMgr*, ObjectType*, WarframeString*, int);
