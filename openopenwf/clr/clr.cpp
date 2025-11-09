#include "../openwf.h"

#include <Shlwapi.h>
#include <MetaHost.h>
#include "clr.h"
#pragma comment(lib, "mscoree.lib")

#include <queue>

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

	OWFLog("Returning a native event");
	return buffer;
}

// CLR -> native
static void SendNativeEvent(unsigned char* buffer, size_t bufferSize)
{
	BinaryReadStream s(buffer, bufferSize);
	std::unique_ptr<NativeEvent> resultEvt;

	switch ((NativeEventId)s.Read<unsigned char>())
	{
		case NativeEventId::RequestTypeList:
			resultEvt = RequestTypeListEvent::Deserialize(s);
			break;
		case NativeEventId::RequestTypeInfo:
			resultEvt = RequestTypeInfoEvent::Deserialize(s);
			break;
		case NativeEventId::RequestSuppressMsgNotify:
			resultEvt = RequestSuppressMsgNotifyEvent::Deserialize(s);
			break;
	}

	OWFLog("Managed event pushed");

	if (!resultEvt)
		return;

	auto lock = eventsLock.Acquire();
	pendingManagedEvents.push(std::move(resultEvt));
}

static void FreeNativeMemory(unsigned char* ptr)
{
	delete ptr;
}

static std::wstring BuildInitArgument()
{
	return std::to_wstring((ULONG_PTR)FreeNativeMemory) + L',' + std::to_wstring((ULONG_PTR)GetNativeEvent) + L',' + std::to_wstring((ULONG_PTR)SendNativeEvent);
}

void InitCLR()
{
	wchar_t dllPath[MAX_PATH];
	if (!GetModuleFileNameW(g_hInstDll, dllPath, std::size(dllPath)))
		FATAL_EXIT(std::format("Could not obtain name of the openopenwf DLL: code {}", GetLastError()));

	wcscpy(PathFindFileNameW(dllPath), L"openopenclr.dll");

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

	OWFLog("CLR successfully initialized"); // horror!
}

void CLRInterop::PushNativeEvent(const BinaryWriteStream& stream)
{
	auto lock = eventsLock.Acquire();

	const auto& origBuffer = stream.GetBuffer();
	std::unique_ptr<unsigned char[]> newBuffer = std::make_unique_for_overwrite<unsigned char[]>(origBuffer.size());
	memcpy(newBuffer.get(), origBuffer.data(), origBuffer.size());

	pendingNativeEvents.push(std::make_pair<std::unique_ptr<unsigned char[]>, size_t>(std::move(newBuffer), origBuffer.size()));
}

std::unique_ptr<NativeEvent> CLRInterop::GetManagedEvent()
{
	auto lock = eventsLock.Acquire();
	if (pendingManagedEvents.empty())
		return nullptr;

	std::unique_ptr<NativeEvent> evt = std::move(pendingManagedEvents.front());
	pendingManagedEvents.pop();
	return evt;
}

void CLRInterop::SendTypeList(const std::unordered_set<std::string>& allTypeList)
{
	BinaryWriteStream ss;

	ss.Write(NativeEventId::ResponseTypeList);
	ss.Write((int)allTypeList.size());
	for (auto&& type : allTypeList)
	{
		ss.Write((int)type.size());
		ss.WriteBytes(type.data(), type.size());
	}

	PushNativeEvent(ss);
}

std::unique_ptr<RequestTypeListEvent> RequestTypeListEvent::Deserialize(BinaryReadStream& stream)
{
	return std::make_unique<RequestTypeListEvent>();
}

std::unique_ptr<RequestTypeInfoEvent> RequestTypeInfoEvent::Deserialize(BinaryReadStream& stream)
{
	return std::make_unique<RequestTypeInfoEvent>();
}

std::unique_ptr<RequestSuppressMsgNotifyEvent> RequestSuppressMsgNotifyEvent::Deserialize(BinaryReadStream& stream)
{
	std::unique_ptr<RequestSuppressMsgNotifyEvent> evt = std::make_unique<RequestSuppressMsgNotifyEvent>();
	evt->shouldSuppress = stream.Read<unsigned char>() != 0;

	return evt;
}
