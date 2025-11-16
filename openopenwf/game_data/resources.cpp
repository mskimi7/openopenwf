#include "resources.h"

#include "../openwf.h"

static std::string GetPropertyTextWrapper(ObjectType* obj, unsigned int flags)
{
	WarframeString pp;
	GetPropertyText(TypeMgr::GetInstance(), obj, &pp, flags);

	return pp.GetText();
}

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

	ResourceInfo rinfo;
	rinfo.type = res.type;
	rinfo.propertyTexts[0x20800001u] = GetPropertyTextWrapper(res.type, 0x20800001u); // relative, own text, no indentation
	rinfo.propertyTexts[0x20800003u] = GetPropertyTextWrapper(res.type, 0x20800003u); // relative, own text, with indentation
	rinfo.propertyTexts[0x21000001u] = GetPropertyTextWrapper(res.type, 0x21000001u); // relative, full text, no indentation
	rinfo.propertyTexts[0x21000003u] = GetPropertyTextWrapper(res.type, 0x21000003u); // relative, full text, with indentation
	rinfo.propertyTexts[0x40800001u] = GetPropertyTextWrapper(res.type, 0x40800001u); // absolute, own text, no indentation
	rinfo.propertyTexts[0x40800003u] = GetPropertyTextWrapper(res.type, 0x40800003u); // absolute, own text, with indentation
	rinfo.propertyTexts[0x41000001u] = GetPropertyTextWrapper(res.type, 0x41000001u); // absolute, full text, no indentation
	rinfo.propertyTexts[0x41000003u] = GetPropertyTextWrapper(res.type, 0x41000003u); // absolute, full text, with indentation
	
	return rinfo;
}
