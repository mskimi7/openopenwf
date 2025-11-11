#pragma once

#include <Windows.h>

class ScopedLock {
	friend class CriticalSectionOwner;

private:
	CRITICAL_SECTION* cs;

	inline explicit ScopedLock(CRITICAL_SECTION* cs_) : cs(cs_) { EnterCriticalSection(this->cs); }

public:
	inline void Release()
	{
		if (this->cs)
		{
			LeaveCriticalSection(this->cs);
			this->cs = nullptr;
		}
	}

	inline ~ScopedLock() { Release(); }
	ScopedLock(ScopedLock const&) = delete;
	ScopedLock& operator=(ScopedLock const&) = delete;

	inline ScopedLock(ScopedLock&& rhs) noexcept { this->cs = rhs.cs; rhs.cs = nullptr; }
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
