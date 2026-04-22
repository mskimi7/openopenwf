#pragma once

template <typename T>
struct WarframeVector {
	T* ptr;
	int usedSize;
	int allocSize;

	size_t size() const { return (size_t)(usedSize / sizeof(T)); }
};
