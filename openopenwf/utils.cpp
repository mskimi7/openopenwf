#include "openwf.h"

#include <wincrypt.h>
#include <bcrypt.h>
#include <winternl.h>

#pragma comment(lib, "crypt32.lib")

static CriticalSectionOwner loggerLock;

void OpenWFLog(const std::string& message)
{
	auto lock = loggerLock.Acquire();

	DWORD charsWritten;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message.data(), (DWORD)message.size(), &charsWritten, nullptr);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1u, &charsWritten, nullptr);
}

void OpenWFLogColor(const std::string& message, unsigned short color)
{
	auto lock = loggerLock.Acquire();

	DWORD charsWritten;
	CONSOLE_SCREEN_BUFFER_INFO cbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbi);
	WORD prevColor = cbi.wAttributes;

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message.data(), (DWORD)message.size(), &charsWritten, nullptr);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), prevColor);
}

__declspec(noreturn) void OpenWFFatalExit(const std::string& reason, const std::string& func, const std::string& file, int line)
{
	std::string msg = std::format("{}\n-------------------------------------------------\nIn: {}\n({}:{})", reason, func, file, line);
	MessageBoxA(nullptr, msg.c_str(), "OpenWF Enabler - Fatal Error", MB_OK | MB_ICONERROR);

	ExitProcess(1);
}

std::string OWFGetBuildLabel()
{
	std::string fullBuildLabel = g_BuildLabelStringPtr;
	size_t spaceIndex = fullBuildLabel.find(' ');
	if (spaceIndex == std::string::npos)
		return fullBuildLabel;

	return fullBuildLabel.substr(0, spaceIndex);
}

std::string Base64Encode(const std::string& inputData)
{
	DWORD requiredLen = 0;
	if (!CryptBinaryToStringA((const unsigned char*)inputData.data(), (DWORD)inputData.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &requiredLen))
		return "";

	std::string result(requiredLen, 0);
	if (!CryptBinaryToStringA((const unsigned char*)inputData.data(), (DWORD)inputData.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, result.data(), &requiredLen))
		return "";

	if (!result.empty() && result.back() == 0)
		result.pop_back();

	return result;
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
