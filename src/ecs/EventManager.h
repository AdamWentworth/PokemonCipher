#pragma once
#include "Entity.h"
#include <functional>
#include <vector>

struct CollisionEvent {
    Entity* entityA = nullptr;
    Entity* entityB = nullptr;
};

//Observer pattern
class EventManager {
public:

    // template<typename EventType>
    // void emit(const EventType& event) {
    //     //retrieve the list of subscribers to certain events
    //     auto& listeners = getListeners<EventType>();
    //     //loop all the subscribers to certain events
    //     for (auto& listener : listeners) {
    //         listener(event);
    //     }
    // }

    void emit(const CollisionEvent& event) const {
        for (const auto& listener: collisionListeners) {
            listener(event);
        }
    }

    // template<typename EventType>
    // void subscribe(std::function<void(const EventType&)> callback) {
    //     getListeners<EventType>().push_back(callback);
    // }

    void subscribe(std::function<void(const CollisionEvent&)> callback) {
        collisionListeners.emplace_back(callback);
    }

private:

    // each event type essentially has its own std::vector of listeners withoutt you having to manage it explictly.
    //This is done using the static local.
    //std:fucntion<void(const EventType&)> is the callable wrapper: can hold anything that can be called like a function
    // template<typename EventType>
    // std::vector<std::function<void(const EventType&)>>& getListeners() {
    //     static std::vector<std::function<void(const EventType&)>> listeners;
    //     return listeners;
    // }

    std::vector<std::function<void(const CollisionEvent&)>> collisionListeners;
};
