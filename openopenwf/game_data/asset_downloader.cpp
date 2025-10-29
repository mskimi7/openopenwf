#include "asset_downloader.h"

#include "../openwf.h"
#include <unordered_set>

struct TreeEntry {
	CompressedTypeName name;
	unsigned char hash[16];
	unsigned int unk;
};

struct TreeEntryVector {
	TreeEntry* ptr;
	TreeEntry* usedSize;
	TreeEntry* allocSize;

	size_t size() const { return (size_t)(usedSize - ptr); }
};

struct TreeBlock {
	TreeBlock* next;
	void* unk1;
	void* unk2;
	unsigned int langSuffixNameIndex;
	unsigned char platform;
	TreeEntryVector data[3];
};

std::vector<std::string> AssetDownloader::GetAllTypes()
{
	std::unordered_set<CompressedTypeName, CompressedTypeName::Hash> allNames;

	EnterCriticalSection(this->GetManifestTreeLock());

	void* manifestTree = this->GetManifestTree();
	for (TreeBlock* treeBlock = *(TreeBlock**)manifestTree; treeBlock != manifestTree; treeBlock = treeBlock->next)
	{
		for (size_t i = 0; i < treeBlock->data[0].size(); ++i)
			allNames.insert(treeBlock->data[0].ptr[i].name);

		for (size_t i = 0; i < treeBlock->data[1].size(); ++i)
			allNames.insert(treeBlock->data[1].ptr[i].name);

		for (size_t i = 0; i < treeBlock->data[2].size(); ++i)
			allNames.insert(treeBlock->data[2].ptr[i].name);
	}

	std::vector<std::string> convertedNames;
	for (auto&& name : allNames)
		convertedNames.push_back(std::string(g_ObjTypeNameMapping->GetName(name.pathIndex)) + g_ObjTypeNameMapping->GetName(name.nameIndex));

	LeaveCriticalSection(this->GetManifestTreeLock());

	return convertedNames;
}
