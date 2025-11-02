#pragma once

#include "../utils/auto_cs.h"

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <unordered_map>

#include <CommCtrl.h>

struct PropertyWindowTypeInfo {
	std::vector<std::string> inheritanceChain; // inheritanceChain[0] always exists and is the requested type's name
	std::string errorMsg; // if type fetching failed, this contains an error message and all other fields are invalid
};

class PropertyWindow {
private:
	HWND hMainWindow = nullptr;
	HWND hTypeTree = nullptr;
	HWND hRefreshList = nullptr;
	HWND hTabView = nullptr;
	HWND hTypeLabel = nullptr;

	bool hasRequestedTypeList = false;
	std::string requestedTypeInfo;

	std::vector<std::string*> typeTreeAllocatedStrings;

	std::unique_ptr<std::vector<std::string>> pendingTypeData;
	std::unique_ptr<PropertyWindowTypeInfo> pendingTypeInfo;
	
	POINT GetOffsetIntoTabControl();

	void UpdateTypeTree();
	void ClearTypeTree();

	void UpdateTypeInfo();

	void RequestReloadTypeList();
	void RequestTypeInfo(const std::string& typeInfo);

	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static void CreateInternal();

	static inline HANDLE hWindowThread;

public:
	static bool IsOpen();
	static void Create();

	static bool ShouldReloadTypes();
	static std::optional<std::string> ShouldFetchTypeInfo();

	static void ReceiveTypeList(std::unique_ptr<std::vector<std::string>> allTypes);
	static void ReceiveTypeInfo(std::unique_ptr<PropertyWindowTypeInfo> typeInfo);

	static inline bool DisableNotify; // this improves performance when deleting a TreeView
	static inline CriticalSectionOwner Lock;
};
