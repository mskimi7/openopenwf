#include "type_mgr.h"

#include "../openwf.h"

std::unique_ptr<std::unordered_set<std::string>> TypeMgr::GetRegisteredTypes()
{
    std::unique_ptr<std::unordered_set<std::string>> result = std::make_unique<std::unordered_set<std::string>>();

    WarframeVector<TypeMgrDirTypeList>* paths = this->GetAllTypePaths();
    for (size_t i = 0; i < paths->size(); ++i)
    {
        std::string path = g_ObjTypeNameMapping->GetName(paths->ptr[i].pathIndex);
        WarframeVector<TypeMgrEntry>* types = &paths->ptr[i].entry->types;

        for (size_t j = 0; j < types->size(); ++j)
            result->insert(path + g_ObjTypeNameMapping->GetName(types->ptr[j].nameIndex));
    }

    return result;
}

TypeMgr* TypeMgr::GetInstance()
{
    return GetTypeMgr();
}
