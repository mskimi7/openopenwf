#include "resources.h"

#include "../openwf.h"

ResourceInfo ResourceMgr::LoadResource(const std::string& fullName)
{
	ObjectSmartPtr objPtr;
	Resource res;
	res.resourcePtr = &objPtr;

	WarframeString resName;
	resName.Create(fullName);

	ResourceMgr::Instance->vtable->AcquireResourceByString(this, &res, &resName, g_BaseType);
	if (!res.type)
		return ResourceInfo();

	WarframeString propertyText;
	GetPropertyText(TypeMgr::GetInstance(), res.type, &propertyText, 0x41000003);

	OWFLog("{} {}", (void*)res.type, propertyText.GetText());

	ResourceInfo rinfo;
	rinfo.type = res.type;
	rinfo.propertyText = propertyText.GetText();
	
	return rinfo;
}
