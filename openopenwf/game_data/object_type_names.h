#pragma once

class ObjectTypeNameMapping {
	struct {
		const char* data;
		size_t totalLength;
	} *segments;

public:
	inline const char* GetName(unsigned int nameIndex) const
	{
		const char* segment = segments[nameIndex & 0xFFFF].data;
		return segment + (nameIndex >> 16);
	}
};

inline ObjectTypeNameMapping* g_ObjTypeNameMapping;
