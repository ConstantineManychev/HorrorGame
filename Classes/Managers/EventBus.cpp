#include "EventBus.h"
#include <algorithm>

EventBus* EventBus::getInstance()
{
    static EventBus instance;
    return &instance;
}

size_t EventBus::subscribe(EventType type, EventCallback callback)
{
    size_t id = ++mNextListenerId;
    mListeners[type].push_back({ id, callback });
    return id;
}

void EventBus::unsubscribe(EventType type, size_t listenerId)
{
    auto it = mListeners.find(type);
    if (it != mListeners.end())
    {
        auto& listeners = it->second;
        // Remove listener with matching ID
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [listenerId](const Listener& l) { return l.id == listenerId; }),
            listeners.end()
        );
    }
}

void EventBus::publish(EventType type, void* data)
{
    auto it = mListeners.find(type);
    if (it != mListeners.end())
    {
        // Iterate over a copy to handle modification during callback safely
        auto listenersCopy = it->second; 
        for (const auto& listener : listenersCopy)
        {
            if (listener.callback)
            {
                listener.callback(data);
            }
        }
    }
}
