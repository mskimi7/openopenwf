#include "type_mgr.h"

#include "../openwf.h"

std::unique_ptr<std::vector<CompressedTypeName>> TypeMgr::GetRegisteredTypes()
{
    std::unique_ptr<std::vector<CompressedTypeName>> allNames = std::make_unique<std::vector<CompressedTypeName>>();

    WarframeVector<TypeMgrDirTypeList>* paths = this->GetAllTypePaths();
    for (size_t i = 0; i < paths->size(); ++i)
    {
        CompressedTypeName ctn = { 0 };
        ctn.pathIndex = paths->ptr[i].pathIndex;

        WarframeVector<TypeMgrEntry>* types = &paths->ptr[i].entry->types;

        for (size_t j = 0; j < types->size(); ++j)
        {
            ctn.nameIndex = types->ptr[j].nameIndex;
            allNames->push_back(ctn);
        }
    }

    return allNames;
}

TypeMgr* TypeMgr::GetInstance()
{
    return GetTypeMgr();
}
