#include "openwf.h"

static CriticalSectionOwner loggerLock;

void OpenWFLog(const std::string& message)
{
	auto lock = loggerLock.Acquire();

	DWORD charsWritten;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message.data(), (DWORD)message.size(), &charsWritten, nullptr);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1u, &charsWritten, nullptr);
}

void OpenWFLogWide(const std::wstring& message)
{
	auto lock = loggerLock.Acquire();

	DWORD charsWritten;
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), message.data(), (DWORD)message.size(), &charsWritten, nullptr);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1u, &charsWritten, nullptr);
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
