//
// Created by genin on 23/09/2026.
// Path: include/libecs/core/view/View.hpp
//

#pragma once

#include <tuple>
#include <type_traits>
#include <cassert>
#include <ranges>
#include <algorithm>

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
        [[nodiscard]] std::array<std::size_t, sizeof...(Components)>
        PoolSizes() const;

        template <std::size_t Pivot, std::size_t... Is, typename Func>
        void EachFrom(Func& func, std::index_sequence<Is...>);
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
    auto View<Components
        ...>::PoolSizes() const -> std::array<
        std::size_t, sizeof...(Components)>
    {
        assert(HasAllPools());

        return std::apply(
            [](auto*... pool) {
                return std::array{ pool->Size()... };
            }, pools_);
    }

    template <typename... Components>
    template <std::size_t Pivot, std::size_t... Is, typename Func>
    void View<Components...>::EachFrom(Func& func, std::index_sequence<Is...>)
    {
        const auto entities = std::get<Pivot>(pools_)->GetEntities();
        for (std::size_t i = entities.size(); i-- > 0;)
        {
            const Entity entity = entities[i];

            // Direct ptr for pivot pool or find lookup
            auto getPtrs = [&]<std::size_t I>() {
                if constexpr (I == Pivot)
                    return &std::get<Pivot>(pools_)->GetByDenseIndex(i);
                else
                    return std::get<I>(pools_)->Find(entity);
            };

            // Fold all pools ptrs to func
            auto process = [&](auto*... ptrs) {
                if ((ptrs && ...))
                {
                    func(entity, *ptrs...);
                }
            };

            process(getPtrs.template operator()<Is>()...);
        }
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

        const auto sizes = PoolSizes();
        const auto pivot = static_cast<std::size_t>(
            std::ranges::min_element(sizes) - sizes.begin());

        auto lambda = [&]<std::size_t I>(const std::size_t index) {
            if (I == index)
            {
                EachFrom<I>(
                    func, std::make_index_sequence<sizeof...(Components)>{});
                return true;
            }
            return false;
        };

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (lambda.template operator()<Is>(pivot) || ...);
        }(std::make_index_sequence<sizeof...(Components)>{});
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

        return std::ranges::min(PoolSizes());
    }
}