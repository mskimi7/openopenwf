#pragma once

template <typename T>
struct WarframeVector {
	T* ptr;
	T* endPtr;
	T* allocPtr;

	size_t size() const { return (size_t)(endPtr - ptr); }
};
