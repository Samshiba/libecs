//
// Created by genin on 04/06/2026.
// Path: include/libecs/core/registry/Registry.hpp
//

#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include <libecs/core/Entity.hpp>
#include <libecs/core/SparseSet.hpp>
#include <libecs/core/registry/IPool.hpp>
#include <libecs/core/registry/TypeId.hpp>
#include <libecs/core/view/View.hpp>

namespace libecs::core::registry
{
    class Registry
    {
    private:
        std::uint32_t nextFreeIndex_ = NULL_INDEX;
        std::vector<Entity> entities_;
        std::vector<std::unique_ptr<IPool> > componentPools_;

        // Pool lookup: nullptr if no component of this type was ever emplaced
        template <typename Component>
        SparseSet<Component>* GetPool();

        template <typename Component>
        const SparseSet<Component>* GetPool() const;

        template <typename Component>
        SparseSet<Component>& GetOrCreatePool();

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

        template <typename Component>
        [[nodiscard]] Component& GetComponent(Entity entity);

        template <typename Component>
        [[nodiscard]] const Component& GetComponent(Entity entity) const;

        template <typename Component>
        [[nodiscard]] Component* TryGetComponent(Entity entity);

        template <typename Component>
        [[nodiscard]] const Component* TryGetComponent(Entity entity) const;

        template <typename... Components>
        [[nodiscard]] view::View<Components...> GetView();
    };

    template <typename Component>
    SparseSet<Component>* Registry::GetPool()
    {
        const ComponentTypeId typeId = GetTypeId<Component>();

        if (typeId >= componentPools_.size())
        {
            return nullptr;
        }

        // Safe: slot typeId only ever holds a SparseSet<Component>
        return static_cast<SparseSet<Component>*>(componentPools_[typeId].
            get());
    }

    template <typename Component>
    const SparseSet<Component>* Registry::GetPool() const
    {
        const ComponentTypeId typeId = GetTypeId<Component>();

        if (typeId >= componentPools_.size())
        {
            return nullptr;
        }

        return static_cast<const SparseSet<Component>*>(componentPools_[
            typeId].get());
    }

    template <typename Component>
    SparseSet<Component>& Registry::GetOrCreatePool()
    {
        const ComponentTypeId typeId = GetTypeId<Component>();

        if (typeId >= componentPools_.size())
        {
            componentPools_.resize(typeId + 1);
        }

        if (componentPools_[typeId] == nullptr)
        {
            componentPools_[typeId] = std::make_unique<SparseSet<Component> >();
        }

        return static_cast<SparseSet<Component>&>(*componentPools_[typeId]);
    }

    template <typename Component, typename... Args>
    Component& Registry::EmplaceComponent(Entity entity, Args&&... args)
    {
        assert(IsEntityValid(entity));

        auto& pool = GetOrCreatePool<Component>();
        pool.Emplace(entity, std::forward<Args>(args)...);

        return pool.Get(entity);
    }

    template <typename Component>
    void Registry::RemoveComponent(Entity entity)
    {
        assert(IsEntityValid(entity));

        auto* pool = GetPool<Component>();

        if (pool && pool->Contains(entity))
        {
            pool->Remove(entity);
        }
    }

    template <typename Component>
    bool Registry::HasComponent(Entity entity) const
    {
        assert(IsEntityValid(entity));

        const auto* pool = GetPool<Component>();
        return pool && pool->Contains(entity);
    }

    template <typename Component>
    Component& Registry::GetComponent(Entity entity)
    {
        assert(HasComponent<Component>(entity));

        return GetPool<Component>()->Get(entity);
    }

    template <typename Component>
    const Component& Registry::GetComponent(Entity entity) const
    {
        assert(HasComponent<Component>(entity));

        return GetPool<Component>()->Get(entity);
    }

    template <typename Component>
    Component* Registry::TryGetComponent(Entity entity)
    {
        assert(IsEntityValid(entity));

        auto* pool = GetPool<Component>();
        return pool && pool->Contains(entity) ? &pool->Get(entity) : nullptr;
    }

    template <typename Component>
    const Component* Registry::TryGetComponent(Entity entity) const
    {
        assert(IsEntityValid(entity));

        const auto* pool = GetPool<Component>();
        return pool && pool->Contains(entity) ? &pool->Get(entity) : nullptr;
    }

    template <typename... Components>
    view::View<Components...> Registry::GetView()
    {
        return view::View<Components...>(GetPool<Components>()...);
    }
}
