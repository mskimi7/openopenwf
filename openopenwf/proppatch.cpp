#include "openwf.h"

static std::vector<std::string> SplitIntoLines(const std::string& text)
{
	std::vector<std::string> result;
	size_t lastStartIdx = 0;

	for (;;)
	{
		size_t currEndIdx = std::min(text.find('\r', lastStartIdx), text.find('\n', lastStartIdx));
		if (currEndIdx == std::string::npos)
		{
			result.push_back(text.substr(lastStartIdx));
			return result;
		}

		result.push_back(text.substr(lastStartIdx, currEndIdx - lastStartIdx));
		lastStartIdx = currEndIdx + 1;
	}
}

static void LoadPatchesFromString(const std::string& contents)
{
	size_t idx = 0;

	// reads string until a delimiter is reached, does not consume the delimiter
	auto readUntil = [&](const std::string& delims, bool throwIfEnd) -> std::string {
		std::string result;
		while (idx < contents.size())
		{
			char c = contents[idx];
			if (delims.find(c) != std::string::npos)
				return result;
			else
				result += c;

			++idx;
		}

		if (throwIfEnd)
			throw std::out_of_range("");

		return result;
	};

	auto peekChar = [&](size_t charsAhead = 0) -> char {
		if ((idx + charsAhead) >= contents.size())
			throw std::out_of_range("");

		return contents[idx + charsAhead];
	};

	auto peekCharOrNul = [&](size_t charsAhead = 0) -> char {
		if ((idx + charsAhead) >= contents.size())
			return 0;

		return contents[idx + charsAhead];
	};

	auto consumeWhitespace = [&]() -> void {
		while (idx < contents.size())
		{
			char c = contents[idx];
			if (c == '#')
			{
				readUntil("\r\n", false);
				continue;
			}

			if (c == '\r' || c == '\n' || c == '\t' || c == ' ')
			{
				++idx;
				continue;
			}

			return;
		}
	};

	try
	{
		for (;;)
		{
			std::vector<std::string> affectedTypes;

			// read types
			for (;;)
			{
				consumeWhitespace();
				if (peekCharOrNul() == '>' || peekCharOrNul() == '&')
				{
					++idx;
					consumeWhitespace();
				}

				affectedTypes.push_back(readUntil("&\r\n\t ", true));
				OWFLog("proppatch [{}]", affectedTypes.back());
				consumeWhitespace();
				if (peekChar() != '&')
					break;

				++idx; // skip the '&'
			}

			// read patches
			for (;;)
			{
				consumeWhitespace();
				if ((peekCharOrNul() == 'r' || peekCharOrNul() == 's' || peekCharOrNul() == 'q') && peekCharOrNul(1) == '|')
				{
					char replacementType = peekCharOrNul(); // 'r', 's' or 'q'
					idx += 2;

					std::string from = readUntil("|", true);
					++idx;
					std::string into = readUntil("\r\n", false);
					consumeWhitespace();

					OWFLog("proppatch repl [{}] [{}] [{}]", replacementType, from, into);
				}
				else if (peekCharOrNul() == '>' || peekCharOrNul() == '&') // start a new patch
				{
					++idx;
					break;
				}
				else if (peekCharOrNul() == '/') // start a new patch (no skipping slash)
				{
					break;
				}
				else
				{
					std::string lineToAdd = readUntil("\r\n", false);
					consumeWhitespace();

					if (!lineToAdd.empty())
					{
						OWFLog("proppatch pref [{}]", lineToAdd);
						// add
					}

					if (idx >= contents.size())
						throw std::out_of_range("");
				}
			}
		}
	}
	catch (const std::out_of_range&) // using exceptions for control flow :)
	{

	}
}

static void LoadPatchesFromFile(const std::wstring& path)
{
	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		OWFLogWide(L"Cannot open metadata patch file: {} (error {})", path, GetLastError());
		return;
	}

	DWORD fileSize = GetFileSize(hFile, nullptr), bytesRead;
	std::string fileContents(GetFileSize(hFile, nullptr), '\0');

	if (!ReadFile(hFile, fileContents.data(), fileSize, &bytesRead, nullptr))
	{
		OWFLogWide(L"Cannot read from metadata patch file: {} (error {})", path, GetLastError());
		CloseHandle(hFile);
		return;
	}

	CloseHandle(hFile);
	OWFLogWide(L"Now loading {}", path);
	LoadPatchesFromString(fileContents);
}

void LoadPropertyTextPatches()
{
	std::wstring patchesDirectory = g_wfExeDirectory + L"OpenWF\\Metadata Patches\\";
	std::wstring searchString = patchesDirectory + L"*.txt";

	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(searchString.c_str(), &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::wstring fileName = patchesDirectory + ffd.cFileName;
		LoadPatchesFromFile(fileName);
	} while (FindNextFileW(hFind, &ffd) != 0);

	FindClose(hFind);
}
