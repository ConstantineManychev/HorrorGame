#ifndef __SERVICE_LOCATOR_H__
#define __SERVICE_LOCATOR_H__

#include "cocos2d.h"
#include <unordered_map>
#include <typeindex>
#include <string>

class ServiceLocator
{
public:
    static ServiceLocator* getInstance();
    static void destroyInstance();

    // Register a service
    template <typename T>
    void registerService(T* service)
    {
        mServices[std::type_index(typeid(T))] = service;
    }

    // Unregister a service
    template <typename T>
    void unregisterService()
    {
        mServices.erase(std::type_index(typeid(T)));
    }

    // Get a service
    template <typename T>
    T* getService()
    {
        auto it = mServices.find(std::type_index(typeid(T)));
        if (it != mServices.end())
        {
            return static_cast<T*>(it->second);
        }
        CCLOG("ServiceLocator: Service %s not found!", typeid(T).name());
        return nullptr;
    }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;
    
    std::unordered_map<std::type_index, void*> mServices;
};

// Helper macro to access the locator easily
#define SL ServiceLocator::getInstance()

#endif // __SERVICE_LOCATOR_H__


