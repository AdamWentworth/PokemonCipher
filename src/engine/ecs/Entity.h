#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <memory>
#include <utility>

#include "ComponentType.h"

constexpr std::size_t MAX_COMPONENTS = 32;
using ComponentBitSet = std::bitset<MAX_COMPONENTS>;

struct ComponentHolderBase {
    virtual ~ComponentHolderBase() = default;
};

template <typename T>
struct ComponentHolder final : ComponentHolderBase {
    T component;

    template <typename... Args>
    explicit ComponentHolder(Args&&... args)
        : component(std::forward<Args>(args)...) {}
};

class Entity {
public:
    bool isActive() const {
        return active_;
    }

    void destroy() {
        active_ = false;
    }

    template <typename T>
    bool hasComponent() const {
        return componentBitSet_[getComponentTypeID<T>()];
    }

    template <typename T, typename... Args>
    T& addComponent(Args&&... args) {
        const ComponentTypeID id = getComponentTypeID<T>();
        auto holder = std::make_unique<ComponentHolder<T>>(std::forward<Args>(args)...);
        T& componentRef = holder->component;

        componentArray_[id] = std::move(holder);
        componentBitSet_[id] = true;

        return componentRef;
    }

    template <typename T>
    T& getComponent() {
        const ComponentTypeID id = getComponentTypeID<T>();
        auto* holder = static_cast<ComponentHolder<T>*>(componentArray_[id].get());
        return holder->component;
    }

    template <typename T>
    const T& getComponent() const {
        const ComponentTypeID id = getComponentTypeID<T>();
        auto* holder = static_cast<const ComponentHolder<T>*>(componentArray_[id].get());
        return holder->component;
    }

private:
    bool active_ = true;
    std::array<std::unique_ptr<ComponentHolderBase>, MAX_COMPONENTS> componentArray_{};
    ComponentBitSet componentBitSet_{};
};
