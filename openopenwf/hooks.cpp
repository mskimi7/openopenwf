#include "openwf.h"
#include "minhook/MinHook.h"

#include <winhttp.h>
#include <Shlwapi.h>
#include <intrin.h>

static decltype(&WinHttpConnect) OLD_WinHttpConnect;
static decltype(&connect) OLD_connect;
static decltype(&getaddrinfo) OLD_getaddrinfo;
static decltype(&GetAddrInfoExW) OLD_GetAddrInfoExW;
static decltype(&SendMessageW) OLD_SendMessageW;
static decltype(&NotifyWinEvent) OLD_NotifyWinEvent;
static void* (*OLD_GameUpdate)(void*);
static int (*OLD_DownloadManifest)(AssetDownloader*, void*, void*, void*, void*, void*);
static int (*OLD_X509_verify_cert)(void*);
static long long (*OLD_Curl_ossl_verifyhost)(void*, void*, void*, void*);
static int (*OLD_WorldStateVerify)(void*, void*);
static int (*OLD_rsa_ossl_public_encrypt)(int, unsigned char*, unsigned char*, void*, int);
static void (*OLD_SendPostRequest_1)(void*, WarframeString*, WarframeString*, char, void*, void*);
static void (*OLD_SendPostRequest_2)(void*, WarframeString*, WarframeString*, char, void*, void*);
static void (*OLD_SendGetRequest_1)(WarframeString*, void*, void*);
static void (*OLD_SendGetRequest_2)(WarframeString*, void*, void*);
static void (*OLD_NRSAnalyze)(void*);
static void* (*OLD_ResourceMgr_Ctor)(ResourceMgr*);

#define REQUEST_TYPE_UNCOMPRESSED 0
#define REQUEST_TYPE_GZIP 1
#define REQUEST_TYPE_GZIP_PROTECTED 2

void WarframeString::Create(const std::string& data)
{
	InitStringFromBytes(this, std::string(data.size(), ' ').c_str());
	memcpy(this->GetPtr(), data.data(), data.size());
}

void WarframeString::Free()
{
	if (this->buf[15] == 0xFF && ((*(unsigned long long*)(this->buf + 8)) & 0xFFFFFFF0000000ull) != 0xFFFFFFF0000000ull)
	{
		WFFree(this->GetPtr());
		this->buf[15] = 0xF;
	}
}

static std::string ReplaceURLHost(const std::string& origURL)
{
	size_t startReplaceIdx = 0;
	int replacementPort = -1;

	if (origURL.starts_with("http://"))
	{
		startReplaceIdx = 7;
		replacementPort = g_Config.httpPort == 80 ? -1 : g_Config.httpPort;
	}
	else if (origURL.starts_with("https://"))
	{
		startReplaceIdx = 8;
		replacementPort = g_Config.httpsPort == 443 ? -1 : g_Config.httpsPort;
	}
	else
		return origURL;

	size_t endReplaceIdx = origURL.find('/', startReplaceIdx);
	if (endReplaceIdx == std::string::npos)
		return origURL;

	std::string replacementHost = g_Config.serverHost;
	if (replacementPort != -1)
		replacementHost += ":"s + std::to_string(replacementPort);

	return origURL.substr(0, startReplaceIdx) + replacementHost + origURL.substr(endReplaceIdx);
}

static INT WSAAPI NEW_getaddrinfo(PCSTR pNodeName, PCSTR pServiceName, ADDRINFOA* pHints, PADDRINFOA* ppResult)
{
	if (pNodeName)
	{
		std::string url = pNodeName;
		for (auto& c : url)
			c = std::tolower(c);

		if (url.find("warframe") != std::wstring::npos)
		{
			OWFLog("[Info] Redirecting getaddrinfo({}, {}) to local...", url, pServiceName ? pServiceName : "<null>");
			return OLD_getaddrinfo(g_Config.serverHost.c_str(), pServiceName, pHints, ppResult);
		}
	}

	return OLD_getaddrinfo(pNodeName, pServiceName, pHints, ppResult);
}

static INT WSAAPI NEW_GetAddrInfoExW(PCWSTR pName, PCWSTR pServiceName, DWORD dwNameSpace, LPGUID lpNspId, const ADDRINFOEXW* hints, PADDRINFOEXW* ppResult,
	timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpHandle)
{
	if (pName)
	{
		std::wstring url = pName;
		for (auto& c : url)
			c = std::tolower(c);

		if (url.find(L"warframe") != std::wstring::npos)
		{
			OWFLog("[Info] Redirecting GetAddrInfoExW({}, {}) to local...", WideToUTF8(url), pServiceName ? WideToUTF8(pServiceName) : "<null>");
			return OLD_GetAddrInfoExW(UTF8ToWide(g_Config.serverHost).c_str(), pServiceName, dwNameSpace, lpNspId, hints, ppResult, timeout, lpOverlapped, lpCompletionRoutine, lpHandle);
		}
	}

	return OLD_GetAddrInfoExW(pName, pServiceName, dwNameSpace, lpNspId, hints, ppResult, timeout, lpOverlapped, lpCompletionRoutine, lpHandle);
}

LRESULT WINAPI NEW_SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (g_DisableWin32NotifyMessages && Msg == WM_NOTIFY)
		return 0;

	return OLD_SendMessageW(hWnd, Msg, wParam, lParam);
}

VOID WINAPI NEW_NotifyWinEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild)
{
	if (g_DisableWin32NotifyMessages && event == EVENT_OBJECT_DESTROY)
		return;

	return OLD_NotifyWinEvent(event, hwnd, idObject, idChild);
}

static int NEW_X509_verify_cert(void* ctx)
{
	return 1;
}

static int NEW_Curl_ossl_verifyhost(void* ctx)
{
	return 0;
}

static int NEW_WorldStateVerify(void* a1, void* a2)
{
	return 1;
}

static int NEW_rsa_ossl_public_encrypt(int flen, unsigned char* from, unsigned char* to, void* rsa, int padding)
{
	if (flen == 40 && padding == 4) // flen = 40 (IV 16 bytes + key 24 bytes)
	{
		memset(from, 0x41, flen); // overwrite the AES key and IV with letters 'A'
		memset(to, 0x41, 128); // overwrite the result of RSA encryption (not needed but... why not)
		return 128;
	}

	return OLD_rsa_ossl_public_encrypt(flen, from, to, rsa, padding);
}

static void NEW_SendPostRequestUnified(decltype(OLD_SendPostRequest_1) origFunc, void* a1, WarframeString* url, WarframeString* bodyData, char requestType, void* a5, void* a6)
{
	WarframeString decryptedData;
	std::string newURL = ReplaceURLHost(url->GetText());

	OWFLog("[POST] {}", url->GetText());

	if (requestType == REQUEST_TYPE_GZIP_PROTECTED)
	{
		if (bodyData->GetSize() > 137) // first byte is some index; subsequent 128 bytes is the RSA-encrypted AES key which we have overwritten; then 8 bytes of useless CRC
		{
			decryptedData.Create(AESDecrypt(bodyData->GetText().substr(137), "AAAAAAAAAAAAAAAAAAAAAAAA", "AAAAAAAAAAAAAAAA"));
			bodyData = &decryptedData;
			requestType = REQUEST_TYPE_GZIP;
		}
		else
		{
			OWFLog("[Warning] protected request is too short ({} bytes)", bodyData->GetSize());
		}
	}

	if (newURL.find("login.php") != std::string::npos)
		newURL += std::format("&buildLabel={}/{}&clientMod={}", OWFGetBuildLabel(), AssetDownloader::Instance->GetCacheManifestHash()->GetText(), REDIRECTOR_NAME);

	WarframeString alteredURL;
	alteredURL.Create(newURL);
	origFunc(a1, &alteredURL, bodyData, requestType, a5, a6);
}

static void NEW_SendPostRequest_1(void* a1, WarframeString* url, WarframeString* bodyData, char requestType, void* a5, void* a6)
{
	return NEW_SendPostRequestUnified(OLD_SendPostRequest_1, a1, url, bodyData, requestType, a5, a6);
}

static void NEW_SendPostRequest_2(void* a1, WarframeString* url, WarframeString* bodyData, char requestType, void* a5, void* a6)
{
	return NEW_SendPostRequestUnified(OLD_SendPostRequest_2, a1, url, bodyData, requestType, a5, a6);
}

static void NEW_SendGetRequestUnified(decltype(OLD_SendGetRequest_1) origFunc, WarframeString* url, void* a2, void* a3)
{
	std::string newURL = ReplaceURLHost(url->GetText());
	OWFLog("[GET] {}", url->GetText());

	if (newURL.find("worldState.php") != std::string::npos)
		newURL += std::format("?buildLabel={}/{}", OWFGetBuildLabel(), AssetDownloader::Instance->GetCacheManifestHash()->GetText());

	WarframeString alteredURL;
	alteredURL.Create(newURL);
	return origFunc(&alteredURL, a2, a3);
}

static void NEW_SendGetRequest_1(WarframeString* url, void* a2, void* a3)
{
	return NEW_SendGetRequestUnified(OLD_SendGetRequest_1, url, a2, a3);
}

static void NEW_SendGetRequest_2(WarframeString* url, void* a2, void* a3)
{
	return NEW_SendGetRequestUnified(OLD_SendGetRequest_2, url, a2, a3);
}

static void* NEW_GameUpdate(void* a1)
{
	if (AssetDownloader::Instance && ResourceMgr::Instance)
	{
		static bool isInspectorLaunched = false;
		if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x50) & 0x8000))
			isInspectorLaunched = false;

		if (!isInspectorLaunched)
		{
			isInspectorLaunched = true;
			CLRInterop::PushNativeEvent(NativeEventId::ResponseShowInspector, "{}");
		}

		auto event = CLRInterop::GetManagedEvent();
		if (!event)
			return OLD_GameUpdate(a1);

		switch (event->GetId())
		{
			case NativeEventId::RequestTypeList:
			{
				RequestTypeListEvent* requestEvt = (RequestTypeListEvent*)event.get();

				std::unique_ptr<std::vector<CompressedTypeName>> manifestTypes = AssetDownloader::Instance->GetManifestTypes(); // all types not in Packages.bin (and others)
				std::unique_ptr<std::vector<CompressedTypeName>> registeredTypes = TypeMgr::GetInstance()->GetRegisteredTypes(); // all types in Packages.bin (and others)

				std::unique_ptr<std::unordered_set<std::string>> stringifiedTypes = std::make_unique<std::unordered_set<std::string>>();

				for (auto&& t : *registeredTypes)
					manifestTypes->push_back(t);

				for (auto&& t : *manifestTypes)
				{
					std::string typeName = g_ObjTypeNameMapping->GetName(t);
					if (!requestEvt->fetchAllTypes)
					{
						if (typeName.find(".") != std::string::npos || typeName.starts_with("/Temp/"))
							continue;
					}
					
					stringifiedTypes->insert(typeName);
				}

				CLRInterop::SendTypeList(*stringifiedTypes);
				break;
			}

			case NativeEventId::RequestTypeInfo:
			{
				RequestTypeInfoEvent* requestEvt = (RequestTypeInfoEvent*)event.get();

				ResourceInfo rinfo = ResourceMgr::Instance->LoadResource(requestEvt->typeName);
				TypeInfoUI typeInfo;
				if (!rinfo.type)
				{
					typeInfo.errorMessage = "Failed to fetch type! Check EE.log for details.";
				}
				else
				{
					ObjectType* tt = rinfo.type;

					do {
						typeInfo.parentTypes.push_back(g_ObjTypeNameMapping->GetName(tt->GetName()));
						tt = tt->parent;
					} while (tt);

					typeInfo.propertyTexts = rinfo.propertyTexts;
				}

				CLRInterop::SendTypeInfo(typeInfo);
				break;
			}

			case NativeEventId::RequestSuppressMsgNotify:
			{
				RequestSuppressMsgNotifyEvent* suppressEvt = (RequestSuppressMsgNotifyEvent*)event.get();
				g_DisableWin32NotifyMessages = suppressEvt->shouldSuppress;

				break;
			}
		}
	}

	return OLD_GameUpdate(a1);
}

static int NEW_DownloadManifest(AssetDownloader* a1, void* a2, void* a3, void* a4, void* a5, void* a6)
{
	AssetDownloader::Instance = a1;
	return 0;
}

static void NEW_NRSAnalyze(void* a1)
{
	if (!g_Config.disableNRS)
		OLD_NRSAnalyze(a1);
}

static void* NEW_ResourceMgr_Ctor(ResourceMgr* resourceMgr)
{
	ResourceMgr::Instance = resourceMgr;
	return OLD_ResourceMgr_Ctor(resourceMgr);
}

template <bool MultipleResultsAllowed = false>
static unsigned char* SignatureScanMustSucceed(const char* pattern, const char* mask, unsigned char* data, size_t length, const char* description)
{
	auto result = SignatureScan(pattern, mask, data, length);
	if ((!MultipleResultsAllowed && result.size() != 1) || (MultipleResultsAllowed && result.size() == 0))
		FATAL_EXIT(std::format("Signature scan failed: a manual update is required.\nWhat failed: {}", description));

	return result[0];
}

void PlaceHooks()
{
	unsigned char* imageBase = (unsigned char*)GetModuleHandleA(nullptr);

	if (!LoadLibraryA("user32.dll"))
		FATAL_EXIT("Failed to load user32.dll");

	if (!LoadLibraryA("ws2_32.dll"))
		FATAL_EXIT("Failed to load ws2_32.dll (Winsock library)");

	if (!LoadLibraryA("winhttp.dll"))
		FATAL_EXIT("Failed to load winhttp.dll (WinHTTP library)");

	MH_Initialize();

	// these two aren't technically necessary but we still hook them for good measure
	MH_CreateHookApi(L"ws2_32.dll", "getaddrinfo", NEW_getaddrinfo, (LPVOID*)&OLD_getaddrinfo);
	MH_CreateHookApi(L"ws2_32.dll", "GetAddrInfoExW", NEW_GetAddrInfoExW, (LPVOID*)&OLD_GetAddrInfoExW);

	// these two hooks (especially the NotifyWinEvent one) massively improve performance of clearing a TreeView in the property window... kinda cursed tho
	MH_CreateHookApi(L"user32.dll", "SendMessageW", NEW_SendMessageW, (LPVOID*)&OLD_SendMessageW);
	MH_CreateHookApi(L"user32.dll", "NotifyWinEvent", NEW_NotifyWinEvent, (LPVOID*)&OLD_NotifyWinEvent);
	
	// A function that gets called repeatedly on the main thread, so that we can achieve thread-safety when accessing game data.
	unsigned char* gameUpdateSig = SignatureScanMustSucceed("\x48\x33\xC4\x48\x89\x45\xF0\x80\x3D\x00\x00\x00\x00\x00\x48\x8B\xF1", "xxxxxxxxx????xxxx", imageBase, 40000000, "GameUpdate");
	gameUpdateSig = (unsigned char*)(((ULONG_PTR)gameUpdateSig - 0x18) & 0xFFFFFFFFFFFFFFF0);
	MH_CreateHook(gameUpdateSig, NEW_GameUpdate, (LPVOID*)&OLD_GameUpdate);

	// H.Cache.bin download function (we pretend it always succeeds so the game uses the file that's already inside cache)
	unsigned char* downloadManifestSig = SignatureScanMustSucceed("\x4C\x8B\xDC\x53\x57\x41\x56\x48\x81\xEC\x90\x01\x00\x00", "xxxxxxxxxxxxxx", imageBase, 40000000, "DownloadManifest");
	MH_CreateHook(downloadManifestSig, NEW_DownloadManifest, (LPVOID*)&OLD_DownloadManifest);

	// SSL certificate validation
	unsigned char* verifyCertSig = SignatureScanMustSucceed("\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x48\x8B\xB9", "xxxxxxxxxxxxx", imageBase, 40000000, "X509_verify_cert");
	MH_CreateHook(verifyCertSig, NEW_X509_verify_cert, (LPVOID*)&OLD_X509_verify_cert);

	// even more SSL certificate validation
	unsigned char* verifyHostSig = SignatureScanMustSucceed("\x49\x8B\x10\x45\x33\xED\x40\x32\xFF", "xxxxxxxxx", imageBase, 40000000, "Curl_ossl_verifyhost");
	verifyHostSig = (unsigned char*)(((ULONG_PTR)verifyHostSig - 0x14) & 0xFFFFFFFFFFFFFFF0);
	MH_CreateHook(verifyHostSig, NEW_Curl_ossl_verifyhost, (LPVOID*)&OLD_Curl_ossl_verifyhost);

	// WorldState signature verification
	unsigned char* worldStateVerifySig = SignatureScanMustSucceed("\x48\x89\x45\xF0\x48\x8B\x00\x84\xD2\x0F\x84\x00\x00\x00\x00\x48\x8D\x15", "xxxxxx?xxxx??xxxxx", imageBase, 40000000, "WorldStateVerify");
	worldStateVerifySig = (unsigned char*)(((ULONG_PTR)worldStateVerifySig - 0x20) & 0xFFFFFFFFFFFFFFF0);
	MH_CreateHook(worldStateVerifySig, NEW_WorldStateVerify, (LPVOID*)&OLD_WorldStateVerify);

	// each protected request has a separate per-request random AES-192 key, which is RSA-encrypted - hook the RSA function to overwrite the random key with a known key & IV
	unsigned char* rsaEncryptSig = SignatureScanMustSucceed("\x4D\x8B\x51\x08\x49\xFF\x62\x08", "xxxxxxxx", imageBase, 40000000, "rsa_ossl_public_encrypt");
	MH_CreateHook(rsaEncryptSig, NEW_rsa_ossl_public_encrypt, (LPVOID*)&OLD_rsa_ossl_public_encrypt);

	// hook SendPostRequest to decrypt the request before sending it to OpenWF (there are two SendPostRequest functions, we hook both)
	std::vector<unsigned char*> sendPostRequest = SignatureScan("\x48\x89\x5C\x24\x20\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x30\x01\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxx", imageBase, 40000000);
	if (sendPostRequest.size() != 2)
		FATAL_EXIT("Signature scan failed: a manual update is required.\nWhat failed: SendPostRequest");

	MH_CreateHook(sendPostRequest[0], NEW_SendPostRequest_1, (LPVOID*)&OLD_SendPostRequest_1);
	MH_CreateHook(sendPostRequest[1], NEW_SendPostRequest_2, (LPVOID*)&OLD_SendPostRequest_2);

	// hook SendGetRequest too
	std::vector<unsigned char*> sendGetRequest = SignatureScan("\x41\xF6\xC0\x04\x74\x02\xCD\x2C", "xxxxxxxx", imageBase, 40000000);
	if (sendGetRequest.size() != 2)
		FATAL_EXIT("Signature scan failed: a manual update is required.\nWhat failed: SendGetRequest");

	MH_CreateHook((char*)(((ULONG_PTR)sendGetRequest[0] - 0x23) & 0xFFFFFFFFFFFFFFF0), NEW_SendGetRequest_1, (LPVOID*)&OLD_SendGetRequest_1);
	MH_CreateHook((char*)(((ULONG_PTR)sendGetRequest[1] - 0x23) & 0xFFFFFFFFFFFFFFF0), NEW_SendGetRequest_2, (LPVOID*)&OLD_SendGetRequest_2);

	// constructor for ResourceMgr
	unsigned char* resourceMgrCtor = SignatureScanMustSucceed("\x80\x61\x58\x80\x48\x8D\x05", "xxxxxxx", imageBase, 40000000, "ResourceMgr::ctor");
	resourceMgrCtor = (unsigned char*)(((ULONG_PTR)resourceMgrCtor - 5) & 0xFFFFFFFFFFFFFFF0);
	MH_CreateHook(resourceMgrCtor, NEW_ResourceMgr_Ctor, (LPVOID*)&OLD_ResourceMgr_Ctor);

	unsigned char* buildLabelSig = SignatureScanMustSucceed("\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xD8\x48\x85\xC0", "xx????xxx??xxx????xxxxxx", imageBase, 40000000, "BuildLabelString");
	buildLabelSig += 2;
	buildLabelSig += *(int*)buildLabelSig;
	buildLabelSig += 5;
	g_BuildLabelStringPtr = (const char*)buildLabelSig;

	unsigned char* initStringFromBytesSig = SignatureScanMustSucceed<true>("\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00",
		"xxx????xxx????x????xxx????xxx????x????xxx????xxx????x????xxx????xxx????x????xxx????xxx????x????xxx????xxx????x????xxx????xxx????x????xxx????xxx????x????", imageBase, 40000000, "InitStringFromBytes");
	initStringFromBytesSig += 15;
	initStringFromBytesSig += *(int*)initStringFromBytesSig;
	initStringFromBytesSig += 4;
	InitStringFromBytes = (decltype(InitStringFromBytes))initStringFromBytesSig;

	unsigned char* wfFreeSig = SignatureScanMustSucceed<true>("\x48\x8B\x4C\x24\x00\x48\x85\xC9\x74\x05\xE8", "xxxx?xxxxxx", imageBase, 40000000, "WFFree");
	wfFreeSig += 11;
	wfFreeSig += *(int*)wfFreeSig;
	wfFreeSig += 4;
	WFFree = (decltype(WFFree))wfFreeSig;

	// NRS analysis (analysis always fails in OpenWF since an NRS server is not available.. yet)
	unsigned char* nrsAnalyzeSig = SignatureScanMustSucceed("\x48\x33\xC4\x48\x89\x85\x00\x00\x00\x00\x83\xB9\x00\x00\x00\x00\x01\x4C\x8B\xE9\x75", "xxxxxx??xxxx??xxxxxxx", imageBase, 40000000, "NRSAnalyze");
	nrsAnalyzeSig = (unsigned char*)(((ULONG_PTR)nrsAnalyzeSig - 0x15) & 0xFFFFFFFFFFFFFFF0);
	MH_CreateHook(nrsAnalyzeSig, NEW_NRSAnalyze, (LPVOID*)&OLD_NRSAnalyze);

	// name mappings for object types (these are stored in contiguous memory blocks and referenced using a pseudo-index)
	unsigned char* typeNameMappingSig = SignatureScanMustSucceed<true>("\x48\x8B\xF9\x48\x8B\x05", "xxxxxx", imageBase, 40000000, "TypeNameMapping");
	typeNameMappingSig += 6;
	typeNameMappingSig += *(int*)typeNameMappingSig;
	typeNameMappingSig += 4;
	g_ObjTypeNameMapping = (ObjectTypeNameMapping*)typeNameMappingSig;

	// pointer to /EE/Types/Base/Object
	unsigned char* baseObjectPtr = SignatureScanMustSucceed("\x48\x89\x45\xD7\x00\xFF\xD1\x4C\x8D\x05", "xxxx?xxxxx", imageBase, 40000000, "BaseObjectPtr");
	baseObjectPtr += 10;
	baseObjectPtr += *(int*)baseObjectPtr;
	baseObjectPtr += 4;
	g_BaseType = (ObjectType*)baseObjectPtr;

	unsigned char* typeMgrPtr = SignatureScanMustSucceed("\xE8\x00\x00\x00\x00\x41\xB9\x03\x00\x00\x21\x4C\x8D\x45\x00\x49\x00\x00\x48\x00\x00\xE8", "x????xxxxxxxxx?x??x??x", imageBase, 40000000, "TypeMgr");
	unsigned char* getPropertyTextPtr = typeMgrPtr;
	typeMgrPtr += 1;
	typeMgrPtr += *(int*)typeMgrPtr;
	typeMgrPtr += 4;
	GetTypeMgr = (decltype(GetTypeMgr))typeMgrPtr;

	getPropertyTextPtr += 22;
	getPropertyTextPtr += *(int*)getPropertyTextPtr;
	getPropertyTextPtr += 4;
	GetPropertyText = (decltype(GetPropertyText))getPropertyTextPtr;

	MH_EnableHook(MH_ALL_HOOKS);
}
