#pragma once

#include <string>
#include <format>

struct CompressedTypeName {
	unsigned int pathIndex;
	unsigned int nameIndex;

	bool operator==(const CompressedTypeName& rhs) const
	{
		return pathIndex == rhs.pathIndex && nameIndex == rhs.nameIndex;
	}

	struct Hash
	{
		size_t operator()(const CompressedTypeName& v) const
		{
			size_t hash = v.nameIndex;
			return (hash << 32) | v.pathIndex;
		}
	};
};

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

	inline std::string GetName(CompressedTypeName nameIndex) const
	{
		return std::format("{}{}", GetName(nameIndex.pathIndex), GetName(nameIndex.nameIndex));
	}
};

inline ObjectTypeNameMapping* g_ObjTypeNameMapping;
