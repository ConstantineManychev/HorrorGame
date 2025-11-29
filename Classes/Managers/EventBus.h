#ifndef __EVENT_BUS_H__
#define __EVENT_BUS_H__

#include "cocos2d.h"
#include "Basics/ServiceLocator.h" // Include ServiceLocator
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

// Define generic Event Types
enum class EventType
{
    // Input Events
    INPUT_ACTION_TRIGGERED, // Data: InputActionData*
    INPUT_AXIS_TRIGGERED,   // Data: InputAxisData*

    // Game State Events
    GAME_PAUSED,
    GAME_RESUMED,
    LEVEL_COMPLETED,
    PLAYER_DEATH,

    // Editor Events
    EDITOR_NODE_SELECTED, // Data: Node*
    EDITOR_MODE_CHANGED,  // Data: bool (casted to void*)
    
    // System
    CONFIG_RELOADED
};

// Data structures for events
struct InputActionData
{
    int action; // int to avoid dependency on GameAction enum here if possible, or include "CommonDefines.h"
    bool isPressed;
};

struct InputAxisData
{
    int action;
    float value;
};

// Helper struct for callback identification
using EventCallback = std::function<void(void*)>;

class EventBus
{
    friend class ServiceLocator;
    friend class AppDelegate;

public:
    static EventBus* getInstance();

    // Subscribe to an event. Returns a listener ID to allow unsubscription.
    size_t subscribe(EventType type, EventCallback callback);

    // Unsubscribe using the ID returned from subscribe
    void unsubscribe(EventType type, size_t listenerId);

    // Publish an event with optional data (synchronous)
    void publish(EventType type, void* data = nullptr);

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    struct Listener
    {
        size_t id;
        EventCallback callback;
    };

    std::unordered_map<EventType, std::vector<Listener>> mListeners;
    size_t mNextListenerId = 0;
};

// Macro for easy access
#define EB SL->getService<EventBus>()

#endif // __EVENT_BUS_H__
