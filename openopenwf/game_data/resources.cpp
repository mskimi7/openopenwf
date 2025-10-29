#include "resources.h"

#include "../openwf.h"

void ResourceMgr::LoadResource(const std::string& fullName)
{
	ObjectSmartPtr objPtr;
	Resource res;
	res.resourcePtr = &objPtr;

	WarframeString resName;
	resName.Create(fullName);

	OWFLog("{}", AssetDownloader::Instance->GetManifestTree());
	auto typeVec = AssetDownloader::Instance->GetAllTypes();
	for (size_t i = 0; i < typeVec.size(); ++i)
	{
		if (typeVec[i].ends_with("bk2"))
			OWFLog("{}", typeVec[i]);
	}

	ResourceMgr::Instance->vtable->AcquireResourceByString(this, &res, &resName, g_BaseType);

	WarframeString propertyText;
	GetPropertyText(GetTypeMgr(), res.type, &propertyText, 0x41000003);

	OWFLog("{}", propertyText.GetText());
}
