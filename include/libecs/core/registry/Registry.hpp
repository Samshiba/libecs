//
// Created by genin on 04/06/2026.
// Path: include/libecs/core/registry/Registry.hpp
//

#pragma once

#include <memory>
#include <vector>
#include <stdexcept>

#include <libecs/core/Entity.hpp>
#include <libecs/core/registry/IPool.hpp>

#include "TypeId.hpp"
#include "libecs/core/SparseSet.hpp"

namespace libecs::core::registry
{
    class Registry
    {
    private:
        std::uint32_t nextFreeIndex_ = NULL_INDEX;
        std::vector<Entity> entities_;
        std::vector<std::unique_ptr<IPool> > componentPools_;

    public:
        Registry();

        Entity CreateEntity();
        bool DestroyEntity(Entity entity);

        [[nodiscard]] bool IsEntityValid(Entity entity) const;

        template <typename Component, typename... Args>
        Component& EmplaceComponent(Entity entity, Args&&... args);

        template <typename Component>
        void RemoveComponent(Entity entity);

        template <typename Component>
        [[nodiscard]] bool HasComponent(Entity entity) const;
    };

    template <typename Component, typename... Args>
    Component& Registry::EmplaceComponent(Entity entity, Args&&... args)
    {
#ifndef NODEBUG
        assert(IsEntityValid(entity));
#endif

        ComponentTypeId typeId = GetTypeId<Component>();

        if (componentPools_.size() <= typeId)
        {
            componentPools_.resize(typeId + 1);
        }

        if (componentPools_[typeId] == nullptr)
        {
            componentPools_[typeId] = std::make_unique<SparseSet<Component> >();
        }

        auto& pool = static_cast<SparseSet<Component>&>(*componentPools_[
            typeId]);
        Component component(std::forward<Args>(args)...);

        pool.Insert(entity, std::move(component));

        return pool.Get(entity);
    }

    template <typename Component>
    void Registry::RemoveComponent(Entity entity)
    {
#ifndef NODEBUG
        assert(IsEntityValid(entity));
#endif
        ComponentTypeId typeId = GetTypeId<Component>();

        if (typeId < componentPools_.size() && componentPools_[typeId] !=
            nullptr)
        {
            static_cast<SparseSet<Component>&>(*componentPools_[typeId]).Remove(
                entity);
        }
    }

    template <typename Component>
    bool Registry::HasComponent(Entity entity) const
    {
#ifndef NODEBUG
        assert(IsEntityValid(entity));
#endif
        ComponentTypeId typeId = GetTypeId<Component>();

        if (componentPools_.size() <= typeId || componentPools_[typeId] ==
            nullptr)
        {
            return false;
        }

        return static_cast<SparseSet<Component>&>(*componentPools_[typeId]).
            Contains(entity);
    }
}