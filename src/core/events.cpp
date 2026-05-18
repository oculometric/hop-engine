#include "events.h"

using namespace HopEngine;

void EventServer::subscribe(TypeID event, Callback callback, void* instance)
{
    auto sub = getInstance()->findSubscriber(event, instance);
    if (sub != getInstance()->subscribers.end() && instance != nullptr)
        DBG_WARNING("event server already has a subscriber registered for event ID " +
                    std::to_string(event) + " to instance " + PTR(instance));
    else
        getInstance()->subscribers.emplace(event, Subscriber{ callback, instance });
}

void EventServer::unsubscribe(TypeID event, void* instance)
{
    auto sub = getInstance()->findSubscriber(event, instance);
    if (sub == getInstance()->subscribers.end())
        DBG_WARNING("event server did not contain a matching subscriber registered for event ID " +
                    std::to_string(event) + " to instance " + PTR(instance));
    else
    {
        do
        {
            getInstance()->subscribers.erase(sub);
            sub = getInstance()->findSubscriber(event, instance);
        } while (sub != getInstance()->subscribers.end());
    }
}

void EventServer::dispatch(TypeID event, void* data, size_t size)
{
    auto [begin, end] = getInstance()->subscribers.equal_range(event);
    while (begin != end)
    {
        if (begin->second.callback) begin->second.callback(data, size, begin->second.instance);
        ++begin;
    }
}

std::multimap<EventServer::TypeID, EventServer::Subscriber>::iterator EventServer::findSubscriber(
    TypeID event, void* instance)
{
    auto [begin, end] = getInstance()->subscribers.equal_range(event);
    while (begin != end)
    {
        if (begin->second.instance == instance) return begin;
        ++begin;
    }
    return getInstance()->subscribers.end();
}
