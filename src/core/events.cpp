#include "events.h"

using namespace HopEngine;
using namespace std;

static EventServer* server = nullptr;

void EventServer::init()
{
    server = new EventServer();
}

void EventServer::destroy()
{
    delete server;
    server = nullptr;
}

void EventServer::subscribe(TypeID event, Callback callback, void* instance)
{
    auto sub = server->findSubscriber(event, instance);
    if (sub != server->subscribers.end() && instance != nullptr)
        DBG_WARNING("event server already has a subscriber registered for event ID " + ::to_string(event) + " to instance " + PTR(instance));
    else
        server->subscribers.emplace(event, Subscriber{ callback, instance });
}

void EventServer::unsubscribe(TypeID event, void* instance)
{
    auto sub = server->findSubscriber(event, instance);
    if (sub == server->subscribers.end())
        DBG_WARNING("event server did not contain a matching subscriber registered for event ID " + ::to_string(event) + " to instance " + PTR(instance));
    else
    {
        do
        {
            server->subscribers.erase(sub);
            sub = server->findSubscriber(event, instance);
        } while (sub != server->subscribers.end());
    }
}

void EventServer::dispatch(TypeID event, void* data, size_t size)
{
    auto [begin, end] = server->subscribers.equal_range(event);
    while (begin != end)
    {
        if (begin->second.callback)
            begin->second.callback(data, size, begin->second.instance);
        ++begin;
    }
}

multimap<EventServer::TypeID, EventServer::Subscriber>::iterator HopEngine::EventServer::findSubscriber(TypeID event, void* instance)
{
    auto [begin, end] = server->subscribers.equal_range(event);
    while (begin != end)
    {
        if (begin->second.instance == instance)
            return begin;
        ++begin;
    }
    return server->subscribers.end();
}
