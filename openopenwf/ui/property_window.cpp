#include "property_window.h"

void PropertyWindow::CreateInternal()
{
}

inline bool PropertyWindow::IsOpen()
{
	auto lock = PropertyWindow::Lock.Acquire();

	if (!PropertyWindow::hWindowThread)
		return false;

	if (WaitForSingleObject(PropertyWindow::hWindowThread, 0) == WAIT_OBJECT_0)
		PropertyWindow::hWindowThread = nullptr;
		
	return PropertyWindow::hWindowThread != nullptr;
}

inline void PropertyWindow::Create()
{
	auto lock = PropertyWindow::Lock.Acquire();

	if (PropertyWindow::IsOpen())
		PropertyWindow::hWindowThread = CreateThread(nullptr, 0, [](LPVOID lpArg) { PropertyWindow::CreateInternal(); return 0ul; }, nullptr, 0, nullptr);
}
