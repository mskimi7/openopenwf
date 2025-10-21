#include "openwf.h"

#include <winternl.h>

/* NASM file:
bits 64

sub rsp, 0x28
xor ecx, ecx ; 1st arg
xor edx, edx ; 2nd arg
lea r8, [rel dll_path] ; 3rd arg (pointer to UNICODE_STRING with our DLL path)
lea r9, [rsp + 0x20] ; 4th arg (not used but must be valid)
mov rax, [rel ldrloaddll_addr]
call rax
add rsp, 0x28
retn

ldrloaddll_addr: dq 0xCCCCCCCCCCCCCCCC ; here goes pointer to LdrLoadDll

dll_path: ;; this is UNICODE_STRING (3rd arg for LdrLoadDll)
dw 0xC3C3 ; USHORT Length
dw 0xCCCC ; USHORT MaximumLength
dd 0 ; padding
dq 0xCCCCCCCCCCCCCCCC ; PWSTR Buffer
*/
static unsigned char injectedCode[] = {
	0x48, 0x83, 0xEC, 0x28, 0x31, 0xC9, 0x31, 0xD2, 0x4C, 0x8D, 0x05, 0x1B,
	0x00, 0x00, 0x00, 0x4C, 0x8D, 0x4C, 0x24, 0x20, 0x48, 0x8B, 0x05, 0x07,
	0x00, 0x00, 0x00, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x28, 0xC3, 0xCC, 0xCC,
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xC3, 0xC3, 0xCC, 0xCC, 0x00, 0x00,
	0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
};

bool LaunchWarframe(HWND mainWindow, const std::wstring& wfExePath, const std::wstring& dllPath, const std::wstring& commandLineArgs)
{
	std::wstring commandLine = wfExePath;

	// wrap in quotation marks in case path contains spaces
	if (!commandLine.starts_with(L'"'))
		commandLine = L'"' + commandLine;

	if (!commandLine.ends_with(L'"'))
		commandLine += L'"';

	commandLine += L" " + commandLineArgs;

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	si.cb = sizeof(si);

	BOOL result = CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi);
	if (!result)
	{
		MessageBoxW(mainWindow, std::format(L"CreateProcess failed (error {})\nAre you sure the Warframe path is correct?", GetLastError()).c_str(), L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return false;
	}

	LPVOID wfPathMem = VirtualAllocEx(pi.hProcess, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!wfPathMem)
	{
		MessageBoxW(mainWindow, std::format(L"VirtualAllocEx failed (error {})\nTry running the OpenWF Enabler as administrator.", GetLastError()).c_str(), L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return false;
	}

	*(LPVOID*)(injectedCode + 0x22) = GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrLoadDll");
	*(USHORT*)(injectedCode + 0x2A) = (USHORT)(dllPath.size() * 2);
	*(USHORT*)(injectedCode + 0x2C) = (USHORT)(dllPath.size() * 2);
	*(char**)(injectedCode + 0x32) = (char*)wfPathMem + sizeof(injectedCode);

	WriteProcessMemory(pi.hProcess, wfPathMem, injectedCode, sizeof(injectedCode), nullptr);
	WriteProcessMemory(pi.hProcess, (char*)wfPathMem + sizeof(injectedCode), dllPath.data(), dllPath.size() * 2, nullptr);

	HANDLE hInjectedThread = CreateRemoteThread(pi.hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)wfPathMem, nullptr, 0, nullptr);
	if (!hInjectedThread)
	{
		MessageBoxW(mainWindow, std::format(L"CreateRemoteThread failed (error {})\nTry running the OpenWF Enabler as administrator.", GetLastError()).c_str(), L"OpenWF Enabler", MB_OK | MB_ICONERROR);
		return false;
	}

	WaitForSingleObject(hInjectedThread, INFINITE);
	CloseHandle(hInjectedThread);

	VirtualFreeEx(pi.hProcess, wfPathMem, 0, MEM_RELEASE);

	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return true;
}
