#pragma once

#include <string>

struct ResourceMgr;
struct WarframeString;

struct ObjectType {

};

inline ObjectType* g_BaseType; // ultimate parent of all types

// Frankly I have no idea what's going on here, it looks like some reference counted pointer
struct ObjectSmartPtr {
	void* ptr = nullptr;
	int unk = 0;
	int refCount = 0;
};

struct Resource {
	ObjectSmartPtr* resourcePtr;
	unsigned char unk[64] = { 0 }; // idk how big the struct actually is, so let's just allocate enough memory to prevent stack corruption
};

typedef void(*AcquireResourceByString_t)(ResourceMgr* resMgr, Resource* outType, WarframeString* objectName, ObjectType* requestedObjectType);

struct ResourceMgrVTable {
	void* func0; // add resource reference?
	void* func1; // remove resource reference?
	void* AcquireResourceById;
	void* AcquireResourceByUnk1;
	void* AcquireResourceByUnk2;
	AcquireResourceByString_t AcquireResourceByString;
	void* AcquireResourceByUnk4;
	void* func7;
	void* func8;
	void* func9;
	void* func10;
	void* func11;
};

struct ResourceMgr {
	ResourceMgrVTable* vtable;

	void LoadResource(const std::string& fullName);
	static inline ResourceMgr* Instance;
};
