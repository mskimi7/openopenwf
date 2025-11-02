#pragma once

// Common Warframe types that are unlikely to cause circular dependencies, and are safe to include in definitions of other types

#include "common/warframe_string.h"
#include "common/warframe_vector.h"
#include "common/compressed_type_name.h"

struct AssetDownloader;
struct TypeMgr;
struct ResourceMgr;
struct ObjectType;

inline ObjectType* g_BaseType; // ultimate parent of all types

#define MEMBER_OFFSET(type, offset) ((type)((char*)this + (offset)))
