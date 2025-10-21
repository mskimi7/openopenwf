#include "openwf.h"

#include <bcrypt.h>
#include <winternl.h>

void OpenWFLog(const std::string& message)
{
	DWORD charsWritten;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message.data(), (DWORD)message.size(), &charsWritten, nullptr);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1u, &charsWritten, nullptr);
}

__declspec(noreturn) void OpenWFFatalExit(const std::string& reason, const std::string& func, const std::string& file, int line)
{
	std::string msg = std::format("{}\n-------------------------------------------------\nIn: {}\n({}:{})", reason, func, file, line);
	MessageBoxA(nullptr, msg.c_str(), "OpenWF Enabler - Fatal Error", MB_OK | MB_ICONERROR);

	ExitProcess(1);
}

std::wstring UTF8ToWide(const std::string& s)
{
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	if (wideLen == 0)
		return L"";

	std::wstring wide(wideLen, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &wide[0], wideLen);

	return wide;
}

std::string WideToUTF8(const std::wstring& s)
{
	int mbLen = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
	if (mbLen == 0)
		return "";

	std::string multibyte(mbLen, 0);
	WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &multibyte[0], mbLen, nullptr, nullptr);

	return multibyte;
}

std::string AESDecrypt(const std::string& inputData, const std::string& key, const std::string& iv)
{
	BCRYPT_ALG_HANDLE algo = 0;
	NTSTATUS ntStatus = BCryptOpenAlgorithmProvider(&algo, BCRYPT_AES_ALGORITHM, nullptr, 0);
	if (!NT_SUCCESS(ntStatus))
	{
		OWFLog("[Error] BCryptOpenAlgorithmProvider failed: NtStatus {:08x}", (unsigned long)ntStatus);
		return "";
	}

	ntStatus = BCryptSetProperty(algo, BCRYPT_CHAINING_MODE, (BYTE*)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
	if (!NT_SUCCESS(ntStatus))
	{
		OWFLog("[Error] BCryptSetProperty(BCRYPT_CHAINING_MODE) failed: NtStatus {:08x}", (unsigned long)ntStatus);
		return "";
	}

	unsigned char keyBlobData[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 32] = { 0 }; // AES key is at most 32 bytes (AES-256)
	BCRYPT_KEY_DATA_BLOB_HEADER* keyBlob = (BCRYPT_KEY_DATA_BLOB_HEADER*)keyBlobData;
	keyBlob->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
	keyBlob->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
	keyBlob->cbKeyData = (ULONG)key.size();
	memcpy(keyBlob + 1, key.data(), key.size());
	
	BCRYPT_KEY_HANDLE keyHandle;
	ntStatus = BCryptImportKey(algo, nullptr, BCRYPT_KEY_DATA_BLOB, &keyHandle, nullptr, 0, keyBlobData, (ULONG)(sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + key.size()), 0);
	if (!NT_SUCCESS(ntStatus))
	{
		OWFLog("[Error] BCryptImportKey failed: NtStatus {:08x}", (unsigned long)ntStatus);
		return "";
	}

	std::string ivCopy = iv;
	std::string output;
	output.resize(inputData.size());

	ULONG bytesWritten = 0;

	ntStatus = BCryptDecrypt(keyHandle, (unsigned char*)inputData.data(), (ULONG)inputData.size(), nullptr, (unsigned char*)ivCopy.data(), (ULONG)ivCopy.size(),
		(unsigned char*)output.data(), (ULONG)output.size(), &bytesWritten, 0);
	if (!NT_SUCCESS(ntStatus))
	{
		OWFLog("[Error] BCryptDecrypt failed: NtStatus {:08x}", (unsigned long)ntStatus);
		return "";
	}

	BCryptDestroyKey(keyHandle);
	BCryptCloseAlgorithmProvider(algo, 0);

	// remove padding
	if (!output.empty())
	{
		unsigned char bytesToRemove = *output.rbegin();
		while (bytesToRemove-- && !output.empty())
			output.pop_back();
	}

	return output;
}
