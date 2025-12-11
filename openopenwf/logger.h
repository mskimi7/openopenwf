#pragma once

#include <string>
#include <format>

void OpenWFLog(const std::string& message);
void OpenWFLogWide(const std::wstring& message);
void OpenWFLogColor(const std::string& message, unsigned short color);
#define OWFLog(fmt, ...) OpenWFLog(std::format(fmt, __VA_ARGS__))
#define OWFLogWide(fmt, ...) OpenWFLogWide(std::format(fmt, __VA_ARGS__))
#define OWFLogColor(color, fmt, ...) OpenWFLogColor(std::format(fmt, __VA_ARGS__), color)
