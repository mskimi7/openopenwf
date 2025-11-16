#include "../openwf.h"

#include <Shlwapi.h>
#include <MetaHost.h>
#include "clr.h"
#pragma comment(lib, "mscoree.lib")

#include <queue>
#include "../json.hpp"

CriticalSectionOwner eventsLock;
std::queue<std::unique_ptr<NativeEvent>> pendingManagedEvents;
std::queue<std::pair<std::unique_ptr<unsigned char[]>, size_t>> pendingNativeEvents;

// native -> CLR
static unsigned char* GetNativeEvent(size_t* bufferSize)
{
	auto lock = eventsLock.Acquire();
	if (pendingNativeEvents.empty())
		return nullptr;

	*bufferSize = pendingNativeEvents.front().second;
	unsigned char* buffer = pendingNativeEvents.front().first.release();
	pendingNativeEvents.pop();

	return buffer;
}

// CLR -> native
static void SendNativeEvent(unsigned char* buffer, size_t bufferSize)
{
	if (bufferSize == 0)
		return;

	std::unique_ptr<NativeEvent> resultEvt;
	NativeEventId eventType = (NativeEventId)buffer[0];

	try
	{
		json eventInfo = json::parse((char*)buffer + 1, (char*)buffer + bufferSize);
		switch (eventType)
		{
			case NativeEventId::RequestTypeList:
				resultEvt = RequestTypeListEvent::Deserialize(eventInfo);
				break;
			case NativeEventId::RequestTypeInfo:
				resultEvt = RequestTypeInfoEvent::Deserialize(eventInfo);
				break;
			case NativeEventId::RequestSuppressMsgNotify:
				resultEvt = RequestSuppressMsgNotifyEvent::Deserialize(eventInfo);
				break;
		}
	}
	catch (const std::exception& ex)
	{
		OWFLog("Exception processing a CLR -> Native event of type {}\n{}", (int)eventType, ex.what());
	}

	if (!resultEvt)
		return;

	auto lock = eventsLock.Acquire();
	pendingManagedEvents.push(std::move(resultEvt));
}

static void FreeNativeMemory(unsigned char* ptr)
{
	delete ptr;
}

static void LogToConsole(const char* msg)
{
	OWFLog("{}", msg);
}

static std::wstring BuildInitArgument()
{
	return std::to_wstring((ULONG_PTR)FreeNativeMemory) + L',' + std::to_wstring((ULONG_PTR)LogToConsole) + L',' + std::to_wstring((ULONG_PTR)GetNativeEvent) +
		L',' + std::to_wstring((ULONG_PTR)SendNativeEvent);
}

void InitCLR()
{
	if (g_Config.disableCLR)
		return;

	wchar_t dllPath[MAX_PATH];
	if (!GetModuleFileNameW(g_hInstDll, dllPath, std::size(dllPath)))
		FATAL_EXIT(std::format("Could not obtain name of the openopenwf DLL: code {}", GetLastError()));

	wcscpy(PathFindFileNameW(dllPath), L"openopenclrrel.dll"); // release build (merged with ilrepack)
	if (!FileExists(dllPath))
		wcscpy(PathFindFileNameW(dllPath), L"openopenclr.dll"); // dev build

	ICLRMetaHost* clrMetaHost = nullptr;
	HRESULT hResult = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&clrMetaHost));
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("CLRCreateInstance failed: code {}", hResult));

	ICLRRuntimeInfo* clrRuntimeInfo = nullptr;
	hResult = clrMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&clrRuntimeInfo)); // this .NET should be installed by default on all Windows machines
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format(".NET GetRuntime v4.0.30319 failed: code {}", hResult));

	BOOL isLoadable = FALSE;
	if (!SUCCEEDED(clrRuntimeInfo->IsLoadable(&isLoadable)) || !isLoadable)
		FATAL_EXIT(".NET Runtime v4.0.30319 cannot be loaded!");

	ICLRRuntimeHost* clrRuntimeHost = nullptr;
	hResult = clrRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&clrRuntimeHost));
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("GetInterface for CLRRuntimeHost failed: code {}", hResult));

	hResult = clrRuntimeHost->Start();
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("CLRRuntimeHost could not be started: code {}", hResult));

	DWORD unused;
	hResult = clrRuntimeHost->ExecuteInDefaultAppDomain(dllPath, L"openopenclr.Program", L"Main", BuildInitArgument().c_str(), &unused);
	if (!SUCCEEDED(hResult))
		FATAL_EXIT(std::format("Cannot load manager library openopenclr.dll: code {}", hResult));
}

void CLRInterop::PushNativeEvent(NativeEventId eventId, const std::string& jsonPayload)
{
	if (g_Config.disableCLR)
		return;

	auto lock = eventsLock.Acquire();

	std::unique_ptr<unsigned char[]> newBuffer = std::make_unique_for_overwrite<unsigned char[]>(jsonPayload.size() + 1);
	newBuffer[0] = (unsigned char)eventId;
	memcpy(newBuffer.get() + 1, jsonPayload.data(), jsonPayload.size());

	pendingNativeEvents.push(std::make_pair<std::unique_ptr<unsigned char[]>, size_t>(std::move(newBuffer), jsonPayload.size() + 1));
}

std::unique_ptr<NativeEvent> CLRInterop::GetManagedEvent()
{
	if (g_Config.disableCLR)
		return nullptr;

	auto lock = eventsLock.Acquire();
	if (pendingManagedEvents.empty())
		return nullptr;

	std::unique_ptr<NativeEvent> evt = std::move(pendingManagedEvents.front());
	pendingManagedEvents.pop();
	return evt;
}

void CLRInterop::SendTypeList(const std::unordered_set<std::string>& allTypeList)
{
	json j = {
		{ "types", allTypeList }
	};

	PushNativeEvent(NativeEventId::ResponseTypeList, j.dump());
}

void CLRInterop::SendTypeInfo(const TypeInfoUI& typeInfo)
{
	std::unordered_map<std::string, std::string> encodedPropertyTexts;
	for (auto&& tt : typeInfo.propertyTexts)
		encodedPropertyTexts[std::to_string(tt.first)] = Base64Encode(tt.second);

	json j = {
		{ "error", typeInfo.errorMessage },
		{ "parentTypes", typeInfo.parentTypes },
		{ "propertyTexts", encodedPropertyTexts }
	};

	PushNativeEvent(NativeEventId::ResponseTypeInfo, j.dump());
}

std::unique_ptr<RequestTypeListEvent> RequestTypeListEvent::Deserialize(const json& j)
{
	std::unique_ptr<RequestTypeListEvent> evt = std::make_unique<RequestTypeListEvent>();
	evt->fetchAllTypes = j.value<bool>("fetchAllTypes", false);

	return evt;
}

std::unique_ptr<RequestTypeInfoEvent> RequestTypeInfoEvent::Deserialize(const json& j)
{
	std::unique_ptr<RequestTypeInfoEvent> evt = std::make_unique<RequestTypeInfoEvent>();
	evt->typeName = j.value<std::string>("typeName", "");

	return evt;
}

std::unique_ptr<RequestSuppressMsgNotifyEvent> RequestSuppressMsgNotifyEvent::Deserialize(const json& j)
{
	std::unique_ptr<RequestSuppressMsgNotifyEvent> evt = std::make_unique<RequestSuppressMsgNotifyEvent>();
	evt->shouldSuppress = j.value<bool>("shouldSuppress", false);

	return evt;
}
