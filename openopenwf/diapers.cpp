#include "openwf.h"

#include <commctrl.h>
#include <shellapi.h>
#include <Objbase.h>
#include <TlHelp32.h>
#include <Psapi.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Comctl32.lib")

static bool TryPatchLaunchScript(const std::wstring& scriptPath, const std::wstring& outputScriptPath)
{
	HANDLE hFile = CreateFileW(scriptPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(hFile, nullptr), readBytes;
		std::string scriptData(fileSize, 0);

		if (ReadFile(hFile, scriptData.data(), fileSize, &readBytes, nullptr))
		{
			/*size_t stashIndex = scriptData.find("git stash");
			if (stashIndex == std::string::npos)
			{
				CloseHandle(hFile);
				return false;
			}

			scriptData.replace(stashIndex, 9, "git stash push -- . \":(exclude)*UPDATE AND START SERVER*\""s);*/

			size_t checkoutIndex = scriptData.find("git checkout");
			if (checkoutIndex == std::string::npos)
			{
				CloseHandle(hFile);
				return false;
			}

			for (/**/; checkoutIndex < scriptData.size(); ++checkoutIndex)
			{
				if (scriptData[checkoutIndex] == '\n')
				{
					++checkoutIndex;
					break;
				}
			}

			scriptData.replace(checkoutIndex, 0, "\tnode -e \"n=`src/controllers/api/loginController.ts`;f=require(`node:fs`);d=f.readFileSync(n,`utf8`);i=d.indexOf(`diapers`)-67;if(i>0){f.writeFileSync(n,d.substring(0,i)+d.substring(i+105))}\"\n");
		}

		CloseHandle(hFile);

		hFile = CreateFileW(outputScriptPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD writtenBytes;

			bool retval = WriteFile(hFile, scriptData.c_str(), (DWORD)scriptData.size(), &writtenBytes, nullptr) != 0;
			CloseHandle(hFile);

			return retval;
		}
	}

	return false;
}

static bool TryPatchScriptsForNode(DWORD nodeProcessId)
{
	OWFLog("Trying node.exe process {}...", nodeProcessId);

	HANDLE hNodeProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, nodeProcessId);
	if (hNodeProcess == nullptr)
		return false;

	bool retval = false;

	// try getting the process's current directory without the ridiculous PEB memory reading
	for (ULONG_PTR remoteHandleId = 0x8; remoteHandleId <= 0x800; remoteHandleId += 4)
	{
		HANDLE hDuplicatedHandle;
		if (DuplicateHandle(hNodeProcess, (HANDLE)remoteHandleId, GetCurrentProcess(), &hDuplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
		{
			DWORD fileType = GetFileType(hDuplicatedHandle);
			if (fileType == FILE_TYPE_DISK)
			{
				wchar_t handleFullPath[512];
				if (GetFinalPathNameByHandleW(hDuplicatedHandle, handleFullPath, std::size(handleFullPath), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS))
				{
					std::wstring handleFullPathStr = handleFullPath;
					if (handleFullPathStr.starts_with(L"\\\\?\\"))
						handleFullPathStr = handleFullPathStr.substr(4);

					if (TryPatchLaunchScript(handleFullPathStr + L"\\UPDATE AND START SERVER.bat", handleFullPathStr + L"\\UPDATE AND START SERVER [openopenwf].bat"))
					{
						OWFLog("Succeeded!");
						retval = true;
					}
				}
			}

			CloseHandle(hDuplicatedHandle);

			if (retval)
				break;
		}
	}

	CloseHandle(hNodeProcess);
	return retval;
}

static bool PatchLaunchScripts()
{
	PROCESSENTRY32W entry = { 0 };
	entry.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return false;

	if (Process32FirstW(hSnapshot, &entry))
	{
		do
		{
			if (_wcsicmp(entry.szExeFile, L"node.exe") == 0)
			{
				if (TryPatchScriptsForNode(entry.th32ProcessID))
					return true;
			}
		} while (Process32NextW(hSnapshot, &entry));
	}

	CloseHandle(hSnapshot);
	return false;
}

void DisplayDiaperWarning()
{
	const static TASKDIALOG_BUTTON buttons[] = {
		{ 20, L"Attempt automated fix (recommended)\nThis will only work if the SpaceNinjaServer is running on the same machine as this Warframe." },
		{ 21, L"Open tutorial page\nOpen a webpage explaining how to fix SpaceNinjaServer to allow access to the latest game version." },
		{ 22, L"Don't do anything\nClose this window without doing anything." }
	};

	TASKDIALOGCONFIG tdConfig = { 0 };
	tdConfig.cbSize = sizeof(TASKDIALOGCONFIG);
	tdConfig.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
	tdConfig.pszMainIcon = TD_INFORMATION_ICON;
	tdConfig.pButtons = buttons;
	tdConfig.cButtons = ARRAYSIZE(buttons);
	tdConfig.pszWindowTitle = L"SpaceNinjaServer patch required";
	tdConfig.pszContent = L"Your SpaceNinjaServer instance does not currently allow connections from the latest game version. To enable this functionality, the server must be patched.";
	tdConfig.pfCallback = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData) -> HRESULT {
		if (msg == TDN_CREATED)
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		return S_OK;
	};

	int selectedMode = 0;
	HRESULT hResult = TaskDialogIndirect(&tdConfig, &selectedMode, NULL, NULL);

	if (selectedMode == 20)
	{
		if (PatchLaunchScripts())
			MessageBoxA(nullptr, "The server has been patched successfully.\nPlease stop SpaceNinjaServer and restart it using the file: UPDATE AND START SERVER [openopenwf].bat", "Patching successful", MB_OK | MB_ICONINFORMATION);
		else
			MessageBoxA(nullptr, "Automated patching failed. Please perform the patching manually.", "Patching failed", MB_OK | MB_ICONWARNING);
	}
	else if (selectedMode == 21)
	{
		if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		{
			ShellExecuteA(nullptr, "open", "https://github.com/mskimi7/openopenwf/blob/master/SERVER_PATCH.md", nullptr, nullptr, SW_SHOWNORMAL);
			CoUninitialize();
		}
	}
}
