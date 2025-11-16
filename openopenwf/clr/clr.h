#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

#include "../utils/auto_cs.h"
#include "../json_fwd.hpp"

using json = nlohmann::json;

// "Request" means "from CLR to native code"
// "Response" means "from native code to CLR"
enum class NativeEventId : unsigned char
{
    RequestTypeList = 0,
    ResponseTypeList = 1,
    RequestTypeInfo = 2,
    ResponseTypeInfo = 3,
    RequestSuppressMsgNotify = 4,
    ResponseShowInspector = 5
};

struct TypeInfoUI {
    std::vector<std::string> parentTypes;
    std::unordered_map<unsigned int, std::string> propertyTexts;
    std::string errorMessage;
};

struct NativeEvent {
    virtual NativeEventId GetId() = 0;
};

struct RequestTypeListEvent : NativeEvent {
    bool fetchAllTypes; // if false, potentially uninteresting types won't be listed

    virtual NativeEventId GetId() override { return NativeEventId::RequestTypeList; }
    static std::unique_ptr<RequestTypeListEvent> Deserialize(const json& j);
};

struct ResponseTypeListEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::ResponseTypeList; }
};

struct RequestTypeInfoEvent : NativeEvent {
    std::string typeName;

    virtual NativeEventId GetId() override { return NativeEventId::RequestTypeInfo; }
    static std::unique_ptr<RequestTypeInfoEvent> Deserialize(const json& j);
};

struct ResponseTypeInfoEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::ResponseTypeInfo; }
};

struct RequestSuppressMsgNotifyEvent : NativeEvent {
    bool shouldSuppress;

    virtual NativeEventId GetId() override { return NativeEventId::RequestSuppressMsgNotify; }
    static std::unique_ptr<RequestSuppressMsgNotifyEvent> Deserialize(const json& j);
};

struct ResponseShowInspectorEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::ResponseShowInspector; }
};

namespace CLRInterop {
    void PushNativeEvent(NativeEventId eventId, const std::string& jsonPayload);
    std::unique_ptr<NativeEvent> GetManagedEvent();

    void SendTypeList(const std::unordered_set<std::string>& allTypeList);
    void SendTypeInfo(const TypeInfoUI& typeInfo);
}
