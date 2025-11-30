#ifndef __EVENT_BUS_H__
#define __EVENT_BUS_H__

#include "cocos2d.h"
#include "Basics/ServiceLocator.h"
#include "EventData.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>

_CSTART

enum class EventType
{
	INPUT_ACTION_TRIGGERED,
	INPUT_AXIS_TRIGGERED,
	GAME_PAUSED,
	GAME_RESUMED,
	LEVEL_COMPLETED,
	PLAYER_DEATH,
	EDITOR_NODE_SELECTED,
	EDITOR_MODE_CHANGED,
	CONFIG_RELOADED
};

using EventCallback = std::function<void(const EventData*)>;

class EventBus
{
	friend class ServiceLocator;
	friend class AppDelegate;

public:
	static EventBus* getInstance();

	template <typename T>
	size_t subscribe(EventType type, std::function<void(const T*)> callback)
	{
		return subscribeInternal(type, [callback, type](const EventData* baseData) {
			const T* typedData = dynamic_cast<const T*>(baseData);
			if (typedData) {
				callback(typedData);
			}
			else if (baseData == nullptr) {
				callback(nullptr);
			}
			else {
				CCLOG("EventBus Error: Data type mismatch for event type %d", (int)type);
			}
		});
	}

	size_t subscribeRaw(EventType type, EventCallback callback);

	void unsubscribe(EventType type, size_t listenerId);

	void publish(EventType type, const EventData* data = nullptr);

private:
	EventBus() = default;
	~EventBus() = default;

	size_t subscribeInternal(EventType type, EventCallback callback);

	struct Listener
	{
		size_t id;
		EventCallback callback;
	};

	std::unordered_map<EventType, std::vector<Listener>> mListeners;
	size_t mNextListenerId = 0;
};

#define EB SL->getService<EventBus>()

_CEND

#endif