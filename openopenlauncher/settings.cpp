#include "openwf.h"

#include <vector>

void SaveLaunchSettings(const std::wstring& warframeExePath, const std::wstring& langCode, bool isDx11)
{
	std::wstring settingFileContents = std::format(L"{}*{}*{}", warframeExePath, langCode, isDx11 ? 1 : 0);
	HANDLE hFile = CreateFileW(g_configFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		std::string settingFileContentsNarrow = WideToUTF8(settingFileContents);
		DWORD written;

		WriteFile(hFile, settingFileContentsNarrow.data(), (DWORD)settingFileContentsNarrow.size(), &written, nullptr);
		CloseHandle(hFile);
	}
}

static std::vector<std::wstring> SplitConfigLine(const std::wstring& settingsLine)
{
	std::vector<std::wstring> result;
	size_t lastStartIdx = 0;

	for (;;)
	{
		size_t currEndIdx = settingsLine.find('*', lastStartIdx);
		if (currEndIdx == std::string::npos)
		{
			result.push_back(settingsLine.substr(lastStartIdx));
			return result;
		}

		result.push_back(settingsLine.substr(lastStartIdx, currEndIdx - lastStartIdx));
		lastStartIdx = currEndIdx + 1;
	}
}

std::optional<LaunchSettings> LoadLaunchSettings()
{
	HANDLE hFile = CreateFileW(g_configFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(hFile, nullptr), readBytes;
		std::string result(fileSize, 0);

		if (ReadFile(hFile, result.data(), fileSize, &readBytes, nullptr))
		{
			std::vector<std::wstring> settingFileContents = SplitConfigLine(UTF8ToWide(result));
			if (settingFileContents.size() != 3)
			{
				CloseHandle(hFile);
				return {};
			}

			LaunchSettings settings;
			settings.warframeExePath = settingFileContents[0];
			settings.langCode = settingFileContents[1];
			settings.isDx11 = settingFileContents[2].empty() || settingFileContents[2][0] != L'0';

			CloseHandle(hFile);
			return settings;
		}

		CloseHandle(hFile);
	}

	return {};
}
