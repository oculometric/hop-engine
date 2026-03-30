#pragma once

#include "common.h"

#include <functional>
#include <map>

namespace HopEngine
{

class EventServer final
{
public:
    typedef uint32_t TypeID;
    typedef std::function<void(void*, size_t, void*)> Callback;

    static constexpr TypeID EVENT_TYPE_NONE = 0x00000000;

private:
    struct Subscriber
    {
        Callback callback = nullptr;
        void* instance = nullptr;
    };

private:
    std::multimap<TypeID, Subscriber> subscribers;

public:
    static void init();
    static void destroy();

    static void subscribe(TypeID event, Callback callback, void* instance);
    static void unsubscribe(TypeID event, void* instance);
    static void dispatch(TypeID event, void* data, size_t size);
    template<typename T> static void dispatch(TypeID event, T data);
    static void dispatch(TypeID event) { dispatch(event, nullptr, 0); }

private:
    EventServer() = default;
    ~EventServer() = default;

    std::multimap<TypeID, Subscriber>::iterator findSubscriber(TypeID event, void* instance);
};

template<typename T> void EventServer::dispatch(TypeID event, T data)
{
    EventServer::dispatch(event, &data, sizeof(T));
}

}