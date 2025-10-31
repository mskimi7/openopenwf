#pragma once

#include <Windows.h>

class ScopedLock {
	friend class CriticalSectionOwner;

private:
	CRITICAL_SECTION* cs;
	bool released = false;

	inline explicit ScopedLock(CRITICAL_SECTION* cs_) : cs(cs_) { EnterCriticalSection(this->cs); }

public:
	inline void Release()
	{
		if (!released)
		{
			released = true;
			LeaveCriticalSection(this->cs);
		}
	}

	inline ~ScopedLock() { Release(); }
	ScopedLock(ScopedLock const&) = delete;
	ScopedLock& operator=(ScopedLock const&) = delete;

	inline ScopedLock(ScopedLock&& rhs) noexcept { this->cs = rhs.cs; rhs.released = true; }
};

class CriticalSectionOwner {
private:
	CRITICAL_SECTION cs;

public:
	inline CriticalSectionOwner() { InitializeCriticalSection(&this->cs); }
	inline ~CriticalSectionOwner() { DeleteCriticalSection(&this->cs); }
	CriticalSectionOwner(CriticalSectionOwner const&) = delete;
	CriticalSectionOwner& operator=(CriticalSectionOwner const&) = delete;

	inline ScopedLock Acquire() { return ScopedLock(&this->cs); }
};
