#include "resources.h"

#include "../openwf.h"

void ResourceMgr::LoadResource(const std::string& fullName)
{
	ObjectSmartPtr objPtr;
	Resource res;
	res.resourcePtr = &objPtr;

	WarframeString resName;
	resName.Create(fullName);

	ResourceMgr::Instance->vtable->AcquireResourceByString(this, &res, &resName, g_BaseType);
}
