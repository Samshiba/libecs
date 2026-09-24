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
        [[nodiscard]] std::size_t SmallestPoolIndex() const;

        template <std::size_t Pivot, typename Func>
        void EachFrom(Func&& func);
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
    std::size_t View<Components...>::SmallestPoolIndex() const
    {
        assert(HasAllPools());

        std::size_t index = 0;
        std::size_t currentIndex = 0;
        std::size_t size = std::get<0>(pools_)->Size();

        auto func = [&](auto pool) {
            if (pool->Size() < size)
            {
                size = pool->Size();
                index = currentIndex;
            }
            currentIndex++;
        };

        std::apply(
            [&](auto*... pool) {
                (func(pool), ...);
            }, pools_);

        return index;
    }

    template <typename... Components>
    template <std::size_t Pivot, typename Func>
    void View<Components...>::EachFrom(Func&& func)
    {
        auto poolPivot = std::get<Pivot>(pools_);
        for (std::size_t i = poolPivot->Size(); i > 0; --i)
        {
            std::size_t index = i - 1;
            auto entity = poolPivot->GetEntities()[index];
            auto pivotComponentPtr = &poolPivot->GetByDenseIndex(index);

            auto invoke = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                // Direct ptr for pivot pool or find lookup
                auto getPtrs = [&]<std::size_t I>() {
                    if constexpr (I == Pivot)
                    {
                        return pivotComponentPtr;
                    }
                    else
                    {
                        return std::get<I>(pools_)->Find(entity);
                    }
                };

                // Fold all pools ptrs to func
                auto process = [&](auto*... ptrs) {
                    if ((ptrs && ...))
                    {
                        func(entity, *ptrs...);
                    }
                };

                process(getPtrs.template operator()<Is>()...);
            };

            invoke(std::make_index_sequence<sizeof...(Components)>{});
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

        std::size_t pivotIndex = SmallestPoolIndex();

        auto lambda = [&]<std::size_t I>(std::size_t index) {
            if (I == index)
            {
                EachFrom<I>(std::forward<Func>(func));
                return true;
            }
            return false;
        };

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (lambda.template operator()<Is>(pivotIndex) || ...);
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

        return SmallestPoolEntities().size();
    }
}