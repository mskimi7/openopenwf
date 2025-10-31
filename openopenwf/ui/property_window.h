#pragma once

#include "../utils/auto_cs.h"

class PropertyWindow {
private:

	static void CreateInternal();
	static inline HANDLE hWindowThread;

public:
	static bool IsOpen();
	static void Create();

	static inline CriticalSectionOwner Lock;
};
