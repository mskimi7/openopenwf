#include "property_window.h"
#include "../openwf.h"

#include <CommCtrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

#include <algorithm>
#include <map>
#include <set>
#include <stack>

static std::unique_ptr<PropertyWindow> window;

POINT PropertyWindow::GetOffsetIntoTabControl()
{
	RECT tvRect;
	GetClientRect(window->hTabView, &tvRect);
	TabCtrl_AdjustRect(window->hTabView, FALSE, &tvRect);

	return { tvRect.left, tvRect.top };
}

void PropertyWindow::RequestReloadTypeList()
{
	auto lock = PropertyWindow::Lock.Acquire();
	this->hasRequestedTypeList = true;
}

void PropertyWindow::UpdateTypeTree()
{
	if (!this->pendingTypeData)
		return;

	auto lock = PropertyWindow::Lock.Acquire();
	if (!this->pendingTypeData)
		return;
	
	this->hasRequestedTypeList = false;
	std::unique_ptr<std::vector<std::string>> allTypes = std::move(this->pendingTypeData);

	lock.Release(); // now we have all required data in local variables, we can release lock while we recreate the window controls

	window->ClearTypeTree();

	// Just a helper struct to organize the type list... probably not so efficient but handles sorting for me
	struct TempTreeEntry {
		std::map<std::string, TempTreeEntry> directories;
		std::set<std::string> files;
	};

	TempTreeEntry rootEntry; // the directory "/"
	for (auto&& type : *allTypes)
	{
		if (!type.starts_with('/'))
		{
			OWFLog("Ignored abnormal type: {}", type);
			continue;
		}

		size_t lastComponentStartIdx = 1;

		TempTreeEntry* currDir = &rootEntry;
		for (;;)
		{
			size_t slashIdx = type.find('/', lastComponentStartIdx);
			if (slashIdx == std::string::npos) // file
			{
				currDir->files.insert(type.substr(lastComponentStartIdx));
				break;
			}
			else // directory
			{
				currDir = &currDir->directories[type.substr(lastComponentStartIdx, slashIdx - lastComponentStartIdx)];
				lastComponentStartIdx = slashIdx + 1;
			}
		}
	}

	auto insertEntries = [&](const auto& self, HTREEITEM parent, const std::string& parentName, const std::string& name, TempTreeEntry* children, int nesting) -> void {
		std::wstring wideName = UTF8ToWide(name);
		std::string fullPath = parentName + name;

		TVINSERTSTRUCTW tviData = { 0 };
		tviData.hParent = parent;
		tviData.item.mask = TVIF_TEXT | TVIF_PARAM;
		tviData.item.lParam = (LPARAM)new std::string(fullPath); // this will be freed when deleting items from this treeview
		tviData.item.pszText = wideName.data();
		tviData.item.cchTextMax = (int)wideName.size() + 1;
		HTREEITEM hEntry = TreeView_InsertItem(this->hTypeTree, &tviData);

		this->typeTreeAllocatedStrings.push_back((std::string*)tviData.item.lParam);

		// for future files
		tviData.hParent = hEntry;
		tviData.hInsertAfter = TVI_LAST;

		if (children)
		{
			for (auto&& childDir : children->directories)
				self(self, hEntry, fullPath, childDir.first + "/", &childDir.second, nesting + 1);

			for (auto&& file : children->files)
			{
				wideName = UTF8ToWide(file);
				tviData.item.lParam = (LPARAM)new std::string(fullPath + file); // this will be freed when deleting items from this treeview
				tviData.item.pszText = wideName.data();
				tviData.item.cchTextMax = (int)wideName.size() + 1;
				HTREEITEM hFileEntry = TreeView_InsertItem(this->hTypeTree, &tviData);

				this->typeTreeAllocatedStrings.push_back((std::string*)tviData.item.lParam);
			}
		}
	};

	SendMessageW(this->hTypeTree, WM_SETREDRAW, FALSE, 0);
	insertEntries(insertEntries, nullptr, "", "/", &rootEntry, 0);
	SendMessageW(this->hTypeTree, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(this->hTypeTree, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void PropertyWindow::ClearTypeTree()
{
	SendMessageW(this->hTypeTree, WM_SETREDRAW, FALSE, 0);
	
	// this lets the NotifyWinEvent hook know to just return early; otherwise each element's removal will call this function
	// and result in a useless system call (Microsoft wtf?)
	PropertyWindow::DisableNotify = true;

	TreeView_DeleteAllItems(this->hTypeTree);

	PropertyWindow::DisableNotify = false;
	SendMessageW(this->hTypeTree, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(this->hTypeTree, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

	for (auto&& ptr : this->typeTreeAllocatedStrings)
		delete ptr;

	this->typeTreeAllocatedStrings.clear();
}

void PropertyWindow::UpdateTypeInfo()
{
	if (!this->pendingTypeInfo)
		return;

	auto lock = PropertyWindow::Lock.Acquire();
	if (!this->pendingTypeInfo)
		return;

	this->requestedTypeInfo.clear();
}

void PropertyWindow::RequestTypeInfo(const std::string& typeInfo)
{
	auto lock = PropertyWindow::Lock.Acquire();
	this->requestedTypeInfo = typeInfo;
}

LRESULT WINAPI PropertyWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
	{
		if (lParam == (LPARAM)window->hRefreshList)
			window->RequestReloadTypeList();
		else
			return DefWindowProcW(hWnd, msg, wParam, lParam);

		return 0;
	}

	case WM_NOTIFY:
	{
		LPNMHDR notifyInfo = (LPNMHDR)lParam;
		if (notifyInfo->hwndFrom == window->hTypeTree && notifyInfo->code == TVN_SELCHANGED)
		{
			LPNMTREEVIEWW tvNotifyInfo = (LPNMTREEVIEWW)lParam;
			std::string* selectedTypeName = (std::string*)tvNotifyInfo->itemNew.lParam;
			if (selectedTypeName && !selectedTypeName->empty() && selectedTypeName->back() != '/') // this shouldn't really ever be null...
			{
				OWFLog("[DEBUG] Requesting info for type: {}", *selectedTypeName);
				window->RequestTypeInfo(*selectedTypeName);
			}
		}

		return 0;
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT WINAPI TransparentLabelWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_CTLCOLORSTATIC)
	{
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void PropertyWindow::CreateInternal()
{
	window = std::make_unique<PropertyWindow>();

	INITCOMMONCONTROLSEX icc = { 0 };
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszClassName = L"oowf_propertywindow";
	RegisterClassExW(&wc);

	window->hMainWindow = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT, wc.lpszClassName, L"OpenWF - Property Text Inspector",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, 931, 459, nullptr, nullptr, wc.hInstance, nullptr);
	
	window->hTypeTree = CreateWindowExW(WS_EX_NOPARENTNOTIFY, WC_TREEVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT, 12, 12, 343, 362, window->hMainWindow, nullptr, wc.hInstance, nullptr);
	window->hRefreshList = CreateWindowExW(0, L"BUTTON", L"Refresh list", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, 240, 380, 115, 28, window->hMainWindow, nullptr, wc.hInstance, nullptr);
	
	window->hTabView = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_HOTTRACK, 361, 12, 542, 396, window->hMainWindow, nullptr, wc.hInstance, nullptr);
	SetWindowSubclass(window->hTabView, TransparentLabelWndProc, 0, 0); // makes labels' backgrounds transparent

	std::wstring tbItemStr;
	TCITEMW tbItem = { 0 };
	tbItem.mask = TCIF_TEXT;

	tbItemStr = L"Type information";
	tbItem.pszText = tbItemStr.data();
	TabCtrl_InsertItem(window->hTabView, 0, &tbItem);

	tbItemStr = L"Property text";
	tbItem.pszText = tbItemStr.data();
	TabCtrl_InsertItem(window->hTabView, 1, &tbItem);

	POINT tabViewOffset = window->GetOffsetIntoTabControl();
	window->hTypeLabel = CreateWindowExW(0, L"STATIC", L"Select a type to view its data.", WS_CHILD | WS_VISIBLE | WS_GROUP, tabViewOffset.x + 6, tabViewOffset.y + 7, 158, 15, window->hTabView, nullptr, wc.hInstance, nullptr);
	
	HFONT hDefFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

	SendMessageW(window->hTypeTree, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(window->hRefreshList, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(window->hTabView, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(window->hTypeLabel, WM_SETFONT, (WPARAM)hDefFont, FALSE);

	ShowWindow(window->hMainWindow, SW_NORMAL);
	UpdateWindow(window->hMainWindow);

	MSG msg;
	BOOL msgSuccess;
	while ((msgSuccess = GetMessageW(&msg, NULL, 0, 0)) != 0)
	{
		if (msgSuccess == -1)
		{
			return;
		}
		else
		{
			if (IsDialogMessageW(window->hMainWindow, &msg) == 0)
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}

		window->UpdateTypeTree();
		window->UpdateTypeInfo();
	}
}

bool PropertyWindow::IsOpen()
{
	auto lock = PropertyWindow::Lock.Acquire();

	if (!PropertyWindow::hWindowThread)
		return false;

	if (WaitForSingleObject(PropertyWindow::hWindowThread, 0) == WAIT_OBJECT_0)
		PropertyWindow::hWindowThread = nullptr;
		
	return PropertyWindow::hWindowThread != nullptr;
}

void PropertyWindow::Create()
{
	auto lock = PropertyWindow::Lock.Acquire();

	if (!PropertyWindow::IsOpen())
		PropertyWindow::hWindowThread = CreateThread(nullptr, 0, [](LPVOID lpArg) { PropertyWindow::CreateInternal(); return 0ul; }, nullptr, 0, nullptr);
}

bool PropertyWindow::ShouldReloadTypes()
{
	auto lock = PropertyWindow::Lock.Acquire();

	return IsOpen() && window->hasRequestedTypeList;
}

std::optional<std::string> PropertyWindow::ShouldFetchTypeInfo()
{
	auto lock = PropertyWindow::Lock.Acquire();

	return (IsOpen() && !window->requestedTypeInfo.empty()) ? std::make_optional(window->requestedTypeInfo) : std::nullopt;
}

void PropertyWindow::ReceiveTypeList(std::unique_ptr<std::vector<std::string>> allTypes)
{
	auto lock = PropertyWindow::Lock.Acquire();

	window->pendingTypeData = std::move(allTypes);
	window->hasRequestedTypeList = false;

	PostMessageW(window->hMainWindow, WM_NULL, 0, 0); // wake up the window so that it processes the type list
}

void PropertyWindow::ReceiveTypeInfo(std::unique_ptr<PropertyWindowTypeInfo> typeInfo)
{
	auto lock = PropertyWindow::Lock.Acquire();

	window->pendingTypeInfo = std::move(typeInfo);
	window->requestedTypeInfo.clear();

	PostMessageW(window->hMainWindow, WM_NULL, 0, 0); // wake up the window so that it processes the type info
}
