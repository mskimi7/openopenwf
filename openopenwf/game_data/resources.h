#pragma once

struct ResourceMgrVTable {
	void* func0; // add resource reference?
	void* func1; // remove resource reference?
	void* AcquireResourceById;
	void* AcquireResourceByUnk1;
	void* AcquireResourceByUnk2;
	void* AcquireResourceByString;
	void* AcquireResourceByUnk4;
	void* func7;
	void* func8;
	void* func9;
	void* func10;
	void* func11;
};

struct ResourceMgr {
	ResourceMgrVTable* vtable;

	static inline ResourceMgr* Instance;
};
