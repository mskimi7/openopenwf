#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

#include "../utils/auto_cs.h"
#include "../utils/stream_read.h"
#include "../utils/stream_write.h"

enum class NativeEventId : unsigned char
{
    RequestTypeList = 0,
    ResponseTypeList = 1,
    RequestTypeInfo = 2,
    ResponseTypeInfo = 3,
    RequestSuppressMsgNotify = 4,
};

template<> inline void BinaryWriteStream::Write(NativeEventId val) { Write((unsigned char)val); }

struct NativeEvent {
    virtual NativeEventId GetId() = 0;
};

struct RequestTypeListEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::RequestTypeList; }
    static std::unique_ptr<RequestTypeListEvent> Deserialize(BinaryReadStream& stream);
};

struct ResponseTypeListEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::ResponseTypeList; }
};

struct RequestTypeInfoEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::RequestTypeInfo; }
    static std::unique_ptr<RequestTypeInfoEvent> Deserialize(BinaryReadStream& stream);
};

struct ResponseTypeInfoEvent : NativeEvent {
    virtual NativeEventId GetId() override { return NativeEventId::ResponseTypeInfo; }
};

struct RequestSuppressMsgNotifyEvent : NativeEvent {
    bool shouldSuppress;

    virtual NativeEventId GetId() override { return NativeEventId::RequestSuppressMsgNotify; }
    static std::unique_ptr<RequestSuppressMsgNotifyEvent> Deserialize(BinaryReadStream& stream);
};

namespace CLRInterop {
    void PushNativeEvent(const BinaryWriteStream& stream);
    std::unique_ptr<NativeEvent> GetManagedEvent();

    void SendTypeList(const std::unordered_set<std::string>& allTypeList);
}
