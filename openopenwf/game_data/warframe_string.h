#pragma once

#include <string>

struct WarframeString {
	unsigned char buf[16] = { 0 };

	inline WarframeString()
	{
		buf[15] = 0x0F;
	}

	inline ~WarframeString()
	{
		Free();
	}

	inline char* GetPtr()
	{
		if (buf[15] == 0xFF)
			return *(char**)buf;

		return (char*)buf;
	}

	inline size_t GetSize() const
	{
		if (buf[15] == 0xFF)
			return *(unsigned int*)(buf + 8) & 0xFFFFFFF;

		return 15 - buf[15];
	}

	inline std::string GetText() const
	{
		const char* strbuf = (const char*)buf;
		int size;

		if (buf[15] == 0xFF)
		{
			strbuf = *(const char**)strbuf;
			size = *(int*)(buf + 8) & 0xFFFFFFF;
		}
		else
		{
			size = 15 - buf[15];
		}

		return std::string(strbuf, size);
	}

	void Create(const std::string& data);
	void Free();
};
