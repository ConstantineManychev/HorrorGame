#include "EventBus.h"
#include <algorithm>

USING_NS_CC;
_CSTART

size_t EventBus::subscribeInternal(EventType type, EventCallback callback)
{
	size_t id = ++mNextListenerId;
	mListeners[type].push_back({ id, callback });
	return id;
}

size_t EventBus::subscribeRaw(EventType type, EventCallback callback)
{
	return subscribeInternal(type, callback);
}

void EventBus::unsubscribe(EventType type, size_t listenerId)
{
	auto it = mListeners.find(type);
	if (it != mListeners.end())
	{
		auto& listeners = it->second;
		listeners.erase(
			std::remove_if(listeners.begin(), listeners.end(),
				[listenerId](const Listener& l) { return l.id == listenerId; }),
			listeners.end()
		);
	}
}

void EventBus::publish(EventType type, const EventData* data)
{
	auto it = mListeners.find(type);
	if (it != mListeners.end())
	{
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

_CEND