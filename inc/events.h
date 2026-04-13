/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"

#include <functional>
#include <cstdint>
#include <map>

namespace HopEngine
{

/**
 * @brief singleton class which handles dispatching and subscribing event callbacks.
 */
class EventServer final
{
public:
    typedef uint32_t TypeID;
    /**
     * callback function pattern. callbacks must return void, and accept three arguments:
     * - void pointer to data supplied by the code invoking the event
     * - size of the data pointed to by the first argument
     * - instance pointer or other user data supplied at the time the subscriber was created
     */
    typedef std::function<void(void*, size_t, void*)> Callback;

    static constexpr TypeID EVENT_TYPE_NONE = 0x00000000;

private:
    /**
     * @brief internal structure for a subscriber instance, detailing the `callback` function and the
     * `instance` value which will be passed to it.
     */
    struct Subscriber
    {
        // static function to be executed when the relevant event is fired
        Callback callback = nullptr;
        // arbitrary data passed to the function upon firing, usually an instance pointer
        void* instance = nullptr;
    };

private:
    std::multimap<TypeID, Subscriber> subscribers; // event ID to subscriber mapping

public:
    static void init();
    static void destroy();

    /**
     * @brief subscribes a new callback to a given event ID.
     * subscribing to the same event multiple times with the same `instance` value is not allowed.
     * execution order for callbacks on the same event is not guaranteed.
     *
     * @param event event type ID for which this callback should be triggered. any value is permitted.
     * @param callback function pointer which will be executed when the event is triggered. cannot be
     * `nullptr`.
     * @param instance user data which will be passed to `callback` upon triggering, usually a pointer
     * to a particular instance intended to be notified (but can be anything). subscribing to the same
     * event ID multiple times with the same `instance` value is not permitted, unless `instance` is
     * `nullptr`.
     */
    static void subscribe(TypeID event, Callback callback, void* instance);
    /**
     * @brief removes a callback which was previously subscribed to a given event ID with a given
     * instance. removes all subscribers which match the `[event, instance]` values. emits an error if
     * there was no known subscriber which matches these values.
     * @param event event type ID for which to search.
     * @param instance user data which was used to subscribe the callback initially.
     */
    static void unsubscribe(TypeID event, void* instance);

    /**
     * @brief dispatches an event for a given event type ID, with some data. all callbacks which are
     * subscribed to the specified event type ID will be executed. execution order of callbacks is not
     * guaranteed.
     * @param event event type ID for which callbacks will be executed. can be any value.
     * @param data pointer to arbitrary amount of data which will be passed to callback functions. may
     * be used to supply information or arguments to callbacks, such as event information structs. can
     * be `nullptr`.
     * @param size size of the data pointed to by `data`, passed to callback functions.
     */
    static void dispatch(TypeID event, void* data, size_t size);
    /**
     * @brief dispatches an event for a given event type ID, with some data. all callbacks which are
     * subscribed to the specified event type ID will be executed. execution order of callbacks is not
     * guaranteed.
     * @param event event type ID for which callbacks will be executed. can be any value.
     * @param data reference to a struct or other type representing information about the event (e.g.
     * the index of which mouse button has just been pressed, etc).
     */
    template<typename T> static void dispatch(TypeID event, const T& data);
    /**
     * @brief dispatches an event for a given event type ID, without data. all callbacks which are
     * subscribed to the specified event type ID will be executed. execution order of callbacks is not
     * guaranteed.
     * @param event event type ID for which callbacks will be executed. can be any value.
     */
    static void dispatch(TypeID event) { dispatch(event, nullptr, 0); }

private:
    EventServer()  = default;
    ~EventServer() = default;

    /**
     * @brief attempts to find a subscriber to a particular event based on the event type ID and the
     * instance data given when subscribing.
     * @param event event type ID for which to match.
     * @param instance user data for which to match.
     * @returns iterator into the `subscribers` map which identifies the first instance of a matching
     * subscriber, or `subscribers->end()` if no matching subscriber was found.
     */
    std::multimap<TypeID, Subscriber>::iterator findSubscriber(TypeID event, void* instance);
};

template<typename T> void EventServer::dispatch(TypeID event, const T& data)
{ EventServer::dispatch(event, &data, sizeof(T)); }

} // namespace HopEngine