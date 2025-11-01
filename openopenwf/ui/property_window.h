#pragma once

#include "../utils/auto_cs.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <CommCtrl.h>

class PropertyWindow {
private:
	HWND hMainWindow = nullptr;
	HWND hTypeTree = nullptr;
	HWND hRefreshList = nullptr;
	HWND hTabView = nullptr;
	HWND hTypeLabel = nullptr;

	bool hasRequestedTypeReload = false;
	std::unique_ptr<std::vector<std::string>> pendingTypeData;
	
	POINT GetOffsetIntoTabControl();

	void RequestReloadTypes();
	void UpdateTypeTree();
	void ClearTypeTree();

	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static void CreateInternal();

	static inline HANDLE hWindowThread;

public:
	static bool IsOpen();
	static void Create();

	static bool ShouldReloadTypes();
	static void PopulateTypeData(std::unique_ptr<std::vector<std::string>> allTypes);

	static inline CriticalSectionOwner Lock;
};
