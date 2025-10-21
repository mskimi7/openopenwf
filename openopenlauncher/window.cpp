#include "openwf.h"

struct LanguageInfo {
	const wchar_t* code;
	const wchar_t* description;
};

constexpr LanguageInfo availableLanguages[] = {
	{ L"de", L"German (de)", },
	{ L"en", L"English (en)", },
	{ L"es", L"Spanish (es)", },
	{ L"fr", L"French (fr)", },
	{ L"it", L"Italian (it)", },
	{ L"ja", L"Japanese (jp)", },
	{ L"ko", L"Korean (ko)", },
	{ L"pl", L"Polish (pl)", },
	{ L"pt", L"Portuguese (pt)", },
	{ L"ru", L"Russian (ru)", },
	{ L"tc", L"Traditional Chinese (tc)", },
	{ L"th", L"Thai (th)", },
	{ L"tr", L"Turkish (tr)", },
	{ L"uk", L"Ukrainian (uk)", },
	{ L"zh", L"Simplified Chinese (zh)", },
};

static HWND mainWindow;
static HWND wfTextbox;
static HWND wfBrowse;
static HWND dllTextbox;
static HWND dllBrowse;
static HWND langCombobox;
static HWND dxCombobox;
static HWND startButton;

static void ShowWarframeFileDialog(LPCWSTR filter, HWND textboxHwnd)
{
	OPENFILENAMEW ofn = { 0 };
	wchar_t pathBuffer[512] = { 0 };

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = pathBuffer;
	ofn.nMaxFile = _countof(pathBuffer);
	ofn.lpstrFilter = filter;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (!GetOpenFileNameW(&ofn))
	{
		if (CommDlgExtendedError() != 0)
		{
			swprintf_s(pathBuffer, L"GetOpenFileNameW failed: error %d", CommDlgExtendedError());
			MessageBoxW(mainWindow, pathBuffer, L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		}

		return;
	}

	SetWindowTextW(textboxHwnd, pathBuffer);
}

static void StartWarframe()
{
	std::wstring warframePath, dllPath, commandLineArgs;
	warframePath.resize(1024);
	dllPath.resize(1024);

	GetWindowTextW(wfTextbox, warframePath.data(), (int)warframePath.size());
	GetWindowTextW(dllTextbox, dllPath.data(), (int)dllPath.size());

	if (warframePath.find(L'\0') != std::wstring::npos) warframePath.resize(warframePath.find(L'\0'));
	if (dllPath.find(L'\0') != std::wstring::npos) dllPath.resize(dllPath.find(L'\0'));

	if (warframePath.empty())
	{
		MessageBoxW(mainWindow, L"Please specify a path to Warframe.x64.exe.", L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return;
	}

	if (dllPath.empty())
	{
		MessageBoxW(nullptr, L"Please specify a path to the OpenWF Enabler library openopenwf.dll.", L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return;
	}

	LRESULT selectedLangIdx = SendMessageW(langCombobox, CB_GETCURSEL, 0, 0);
	LRESULT selectedDxIdx = SendMessageW(dxCombobox, CB_GETCURSEL, 0, 0);

	if (selectedLangIdx == CB_ERR)
	{
		MessageBoxW(nullptr, L"Please select a language.", L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return;
	}

	if (selectedDxIdx == CB_ERR)
	{
		MessageBoxW(nullptr, L"Please select a DirectX version.", L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return;
	}

	commandLineArgs = std::format(L" -cluster:public -allowmultiple -language:{} -graphicsDriver:{}", availableLanguages[selectedLangIdx].code,
		selectedDxIdx == 0 ? L"dx11" : L"dx12");

	EnableWindow(startButton, false);

	if (LaunchWarframe(mainWindow, warframePath, dllPath, commandLineArgs))
		ExitProcess(0);

	EnableWindow(startButton, true);
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
	{
		if (lParam == (LPARAM)wfBrowse)
			ShowWarframeFileDialog(L"Warframe (Warframe.x64.exe)\0Warframe.x64.exe\0All Files (*.*)\0*.*\0\0", wfTextbox);
		else if (lParam == (LPARAM)dllBrowse)
			ShowWarframeFileDialog(L"OpenWF Enabler (openopenwf.dll)\0openopenwf.dll\0All Files (*.*)\0*.*\0\0", dllTextbox);
		else if (lParam == (LPARAM)startButton)
			StartWarframe();
		else
			return DefWindowProcW(hWnd, msg, wParam, lParam);

		return 0;
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void FillDefaults()
{
	HANDLE hDll = CreateFileW(L"openopenwf.dll", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDll != INVALID_HANDLE_VALUE)
	{
		wchar_t dllFullPath[512];
		if (GetFinalPathNameByHandleW(hDll, dllFullPath, std::size(dllFullPath), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS))
		{
			std::wstring dllFullPathStr = dllFullPath;
			if (dllFullPathStr.starts_with(L"\\\\?\\"))
				dllFullPathStr = dllFullPathStr.substr(4);

			SetWindowTextW(dllTextbox, dllFullPathStr.c_str());
		}
	}

	std::optional<std::wstring> languageSetting;
	std::optional<bool> isDx11Setting;

	std::wstring wfPath = GuessWarframeSettings(languageSetting, isDx11Setting);
	if (!wfPath.empty())
	{
		SetWindowTextW(wfTextbox, wfPath.c_str());

		if (languageSetting.has_value())
		{
			for (size_t i = 0; i < std::size(availableLanguages); ++i)
			{
				if (wcscmp(availableLanguages[i].code, languageSetting.value().c_str()) == 0)
				{
					SendMessageW(langCombobox, CB_SETCURSEL, i, 0);
					goto setGraphics;
				}
			}
		}

		SendMessageW(langCombobox, CB_SETCURSEL, 1, 0); // en

	setGraphics:
		SendMessageW(dxCombobox, CB_SETCURSEL, (!isDx11Setting.has_value() || isDx11Setting.value() == true) ? 0 : 1, 0);
	}
}

void CreateLaunchDialog()
{
	INITCOMMONCONTROLSEX icc = { 0 };
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"openopenlauncher_cl";
	RegisterClassExW(&wc);

	mainWindow = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT, wc.lpszClassName, L"OpenWF Enabler",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, 558, 209, nullptr, nullptr, wc.hInstance, nullptr);

	HWND hWarframePathLabel = CreateWindowExW(0, L"STATIC", L"Warframe.x64.exe path:", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_CLIPSIBLINGS, 12, 16, 131, 15, mainWindow, nullptr, nullptr, nullptr);
	wfTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | ES_AUTOHSCROLL, 149, 13, 281, 23, mainWindow, nullptr, nullptr, nullptr);
	wfBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, 436, 13, 94, 23, mainWindow, nullptr, nullptr, nullptr);

	HWND hLogPathLabel = CreateWindowExW(0, L"STATIC", L"openopenwf .dll:", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_GROUP, 46, 45, 103, 15, mainWindow, nullptr, nullptr, nullptr);
	dllTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | ES_AUTOHSCROLL, 149, 42, 281, 23, mainWindow, nullptr, nullptr, nullptr);
	dllBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, 436, 42, 94, 23, mainWindow, nullptr, nullptr, nullptr);
	
	HWND hLanguageLabel = CreateWindowExW(0, L"STATIC", L"Game language:", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_GROUP, 50, 74, 93, 15, mainWindow, nullptr, nullptr, nullptr);
	langCombobox = CreateWindowExW(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_HASSTRINGS, 149, 71, 166, 400, mainWindow, nullptr, nullptr, nullptr);

	HWND hDXLabel = CreateWindowExW(0, L"STATIC", L"DirectX version:", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_GROUP, 54, 103, 89, 15, mainWindow, nullptr, nullptr, nullptr);
	dxCombobox = CreateWindowExW(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_HASSTRINGS, 149, 100, 166, 400, mainWindow, nullptr, nullptr, nullptr);

	startButton = CreateWindowExW(0, L"BUTTON", L"Start Warframe (OpenWF)", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_GROUP | WS_TABSTOP, 12, 129, 518, 29, mainWindow, nullptr, nullptr, nullptr);

	HFONT hDefFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

	SendMessageW(hWarframePathLabel, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(wfTextbox, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(wfBrowse, WM_SETFONT, (WPARAM)hDefFont, FALSE);

	SendMessageW(hLogPathLabel, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(dllTextbox, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(dllBrowse, WM_SETFONT, (WPARAM)hDefFont, FALSE);

	SendMessageW(hLanguageLabel, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(langCombobox, WM_SETFONT, (WPARAM)hDefFont, FALSE);

	SendMessageW(hDXLabel, WM_SETFONT, (WPARAM)hDefFont, FALSE);
	SendMessageW(dxCombobox, WM_SETFONT, (WPARAM)hDefFont, FALSE);

	SendMessageW(startButton, WM_SETFONT, (WPARAM)hDefFont, FALSE);

	for (size_t i = 0; i < std::size(availableLanguages); ++i)
	{
		SendMessageW(langCombobox, CB_ADDSTRING, 0, (LPARAM)availableLanguages[i].description);
		SendMessageW(langCombobox, CB_SETITEMDATA, i, (LPARAM)availableLanguages[i].code);
	}

	SendMessageW(dxCombobox, CB_ADDSTRING, 0, (LPARAM)L"DirectX 11");
	SendMessageW(dxCombobox, CB_ADDSTRING, 0, (LPARAM)L"DirectX 12");

	FillDefaults();

	ShowWindow(mainWindow, SW_NORMAL);
	UpdateWindow(mainWindow);

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
			if (IsDialogMessageW(mainWindow, &msg) == 0)
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
	}
}
