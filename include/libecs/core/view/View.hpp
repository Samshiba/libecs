//
// Created by genin on 23/09/2026.
// Path: include/libecs/core/view/View.hpp
//

#pragma once

#include <tuple>
#include <type_traits>
#include <span>
#include <cassert>
#include <ranges>

#include <libecs/core/SparseSet.hpp>

namespace libecs::core::view
{
    template <typename... Components>
    class View
    {
        using Pools = std::tuple<SparseSet<Components>*...>;

        static_assert(sizeof...(Components) > 0,
                      "View must have at least one component type.");

    public:
        explicit View(SparseSet<Components>*... pools);

        template <typename Func>
        void Each(Func&& func);

        [[nodiscard]] bool Contains(Entity entity) const;

        template <typename Component>
        Component& Get(Entity entity);

        [[nodiscard]] std::size_t MaxSize() const;

    private:
        Pools pools_;

        [[nodiscard]] bool HasAllPools() const;
        [[nodiscard]] std::span<const Entity> SmallestPoolEntities() const;
    };

    template <typename... Components>
    bool View<Components...>::HasAllPools() const
    {
        return std::apply(
            [](auto*... pool) {
                return ((pool != nullptr) && ...);
            }, pools_);
    }

    template <typename... Components>
    std::span<const Entity> View<Components...>::SmallestPoolEntities() const
    {
        assert(HasAllPools());

        auto result = std::get<0>(pools_)->GetEntities();

        auto func = [&result](auto pool) {
            if (pool->Size() < result.size())
            {
                result = pool->GetEntities();
            }
        };

        std::apply(
            [&](auto*... pool) {
                (func(pool), ...);
            }, pools_);
        return result;
    }

    template <typename... Components>
    View<Components...>::View(SparseSet<Components>*... pools)
        : pools_(pools...)
    {
    }

    template <typename... Components>
    template <typename Func>
    void View<Components...>::Each(Func&& func)
    {
        if (!HasAllPools())
            return;

        for (auto entity : SmallestPoolEntities() | std::views::reverse)
        {
            if (Contains(entity))
            {
                // Apply func
                std::apply(
                    [&](auto*... pool) {
                        func(entity, pool->Get(entity)...);
                    }, pools_);
            }
        }
    }

    template <typename... Components>
    bool View<Components...>::Contains(Entity entity) const
    {
        return std::apply(
            [entity](auto*... pool) {
                return ((pool && pool->Contains(entity)) && ...);
            }, pools_);
    }

    template <typename... Components>
    template <typename Component>
    Component& View<Components...>::Get(Entity entity)
    {
        static_assert((std::is_same_v<Component, Components> || ...),
                      "Component must be part of this View");

        return std::get<SparseSet<Component>*>(pools_)->Get(entity);
    }

    template <typename... Components>
    std::size_t View<Components...>::MaxSize() const
    {
        if (!HasAllPools())
            return 0;

        return SmallestPoolEntities().size();
    }
}