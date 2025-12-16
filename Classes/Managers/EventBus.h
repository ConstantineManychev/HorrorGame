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
	CONFIG_RELOADED,
	ENTITY_CREATED
};

using EventCallback = std::function<void(const EventData*)>;

class EventBus
{
	friend class ServiceLocator;
	friend class AppDelegate;

public:

	template <typename T>
	size_t subscribe(EventType aType, std::function<void(const T*)> aCallback)
	{
		return subscribeInternal(aType, [aCallback, aType](const EventData* aBaseData) {
			const T* typedData = dynamic_cast<const T*>(aBaseData);
			if (typedData) {
				aCallback(typedData);
			}
			else if (aBaseData == nullptr) {
				aCallback(nullptr);
			}
			else {
				CCLOG("EventBus Error: Data type mismatch for event type %d", (int)aType);
			}
		});
	}

	size_t subscribeRaw(EventType aType, EventCallback aCallback);

	void unsubscribe(EventType aType, size_t aListenerId);

	void publish(EventType aType, const EventData* aData = nullptr);

private:
	EventBus() = default;
	~EventBus() = default;

	size_t subscribeInternal(EventType aType, EventCallback aCallback);

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