//
// Created by genin on 25/09/2026.
// Path: include/libecs/core/view/ViewIterator.hxx
//

#pragma once

#include <iterator>
#include <cstddef>
#include <span>
#include <tuple>

namespace libecs::core::view
{
    template <typename... Components>
    class View<Components...>::ViewIterator
    {
    public:
        // Def for <ranges> compatibility
        using iterator_concept = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::tuple<Entity, Components&...>;

        ViewIterator(const View& view,
                     std::span<const Entity> entities);

        value_type operator*() const;
        ViewIterator& operator++();
        void operator++(int);

        friend bool operator==(const ViewIterator& it, std::default_sentinel_t)
        {
            return it.remaining_ == 0;
        }

    private:
        View view_;
        std::span<const Entity> entities_;
        std::size_t remaining_;

        void SkipInvalidEntities();
    };

    template <typename... Components>
    void View<Components...>::ViewIterator::SkipInvalidEntities()
    {
        while (remaining_ > 0)
        {
            const Entity entity = entities_[remaining_ - 1];
            if (view_.Contains(entity))
            {
                break;
            }
            --remaining_;
        }
    }

    template <typename... Components>
    View<Components...>::ViewIterator::ViewIterator(
        const View& view, std::span<const Entity> entities)
        : view_(view), entities_(entities)
    {
        remaining_ = entities_.size();
        SkipInvalidEntities();
    }

    template <typename... Components>
    typename View<Components...>::ViewIterator::value_type View<Components...>::
    ViewIterator::operator*() const
    {
        const Entity entity = entities_[remaining_ - 1];
        return std::apply([entity](auto*... pool) {
            return value_type{ entity, *pool->Find(entity)... };
        }, view_.pools_);
    }

    template <typename... Components>
    typename View<Components...>::ViewIterator& View<Components...>::
    ViewIterator::operator++()
    {
        --remaining_;
        SkipInvalidEntities();
        return *this;
    }

    template <typename... Components>
    void View<Components...>::ViewIterator::operator++(int)
    {
        ++(*this);
    }

    template <typename... Components>
    typename View<Components...>::ViewIterator View<Components...>::begin()
    {
        if (!HasAllPools())
            return ViewIterator(*this, {});

        const auto sizes = PoolSizes();
        const auto pivot = static_cast<std::size_t>(
            std::ranges::min_element(sizes) - sizes.begin());

        std::span<const Entity> entities;

        std::size_t index = 0;
        std::apply([&](const auto*... pool) {
            ((index++ == pivot
                ? void(entities = pool->GetEntities())
                : void()), ...);
        }, pools_);

        return ViewIterator(*this, entities);
    }

    template <typename... Components>
    std::default_sentinel_t View<Components...>::end() const
    {
        return {};
    }
}