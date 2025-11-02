#pragma once

#include "common.h"

// Frankly I have no idea what's going on here, it looks like some reference counted pointer
struct ObjectSmartPtr {
	void* ptr = nullptr;
	int unk = 0;
	int refCount = 0;
};

struct Resource {
	ObjectSmartPtr* resourcePtr = nullptr;
	ObjectType* type = nullptr;
};

typedef void(*AcquireResourceByString_t)(ResourceMgr* resMgr, Resource* outType, WarframeString* objectName, ObjectType* requestedObjectType);

struct ResourceInfo {
	ObjectType* type = nullptr;
	std::string propertyText;
};

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

	ResourceInfo LoadResource(const std::string& fullName);
	static inline ResourceMgr* Instance;
};
