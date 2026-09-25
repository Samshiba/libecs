//
// Created by genin on 03/06/2026.
// Path: include/libecs/core/SparseSet.hpp
//

#pragma once

#include <vector>
#include <array>
#include <memory>
#include <cassert>
#include <utility>
#include <span>

#include <libecs/core/Entity.hpp>
#include <libecs/core/registry/IPool.hpp>

namespace libecs::core
{
    template <typename Component>
    class SparseSet : public registry::IPool
    {
    private:
        static constexpr std::size_t PAGE_SIZE = 4096;
        using Page = std::array<std::size_t, PAGE_SIZE>;

        struct SparseLocation
        {
            std::size_t pageIndex;
            std::size_t offset;
        };

        static SparseLocation Locate(Entity entity);

    public:
        SparseSet() = default;
        ~SparseSet() override = default;

        void Insert(Entity entity, Component component);
        void Remove(Entity entity);

        template <typename... Args>
        void Emplace(Entity entity, Args&&... args);

        [[nodiscard]] bool Contains(Entity entity) const;
        Component& Get(Entity entity);
        const Component& Get(Entity entity) const;
        Component* Find(Entity entity);
        const Component* Find(Entity entity) const;

        Component& GetByDenseIndex(std::size_t index);
        const Component& GetByDenseIndex(std::size_t index) const;

        void EntityDestroyed(Entity entity) override;

        [[nodiscard]] std::size_t Size() const;

        [[nodiscard]] std::span<const Entity> GetEntities() const;

    private:
        std::vector<Component> denseComponents_;
        std::vector<Entity> denseEntities_;
        std::vector<std::unique_ptr<Page> > sparse_;
    };

    template <typename Component>
    auto SparseSet<Component>::Locate(Entity entity) -> SparseLocation
    {
        const uint32_t entityIndex = GetEntityIndex(entity);
        return { entityIndex / PAGE_SIZE, entityIndex % PAGE_SIZE };
    }

    template <typename Component>
    void SparseSet<Component>::Insert(Entity entity, Component component)
    {
        Emplace(entity, std::move(component));
    }

    template <typename Component>
    void SparseSet<Component>::Remove(Entity entity)
    {
        assert(Contains(entity));

        auto [pageIndex, offset] = Locate(entity);
        std::size_t denseIndex = (*sparse_[pageIndex])[offset];
        Entity lastEntity = denseEntities_.back();

        // Swap the component to be removed with the last component in denses
        std::swap(denseComponents_[denseIndex], denseComponents_.back());
        std::swap(denseEntities_[denseIndex], denseEntities_.back());
        denseComponents_.pop_back();
        denseEntities_.pop_back();

        // Update the sparse set to point to the new index of the moved component
        if (entity != lastEntity)
        {
            auto [lastPageIndex, lastOffset] = Locate(lastEntity);
            (*sparse_[lastPageIndex])[lastOffset] = denseIndex;
        }
    }

    template <typename Component>
    template <typename... Args>
    void SparseSet<Component>::Emplace(Entity entity, Args&&... args)
    {
        if (Contains(entity))
        {
            Get(entity) = Component(std::forward<Args>(args)...);
            return;
        }

        denseComponents_.emplace_back(std::forward<Args>(args)...);
        denseEntities_.push_back(entity);
        std::size_t denseIndex = denseComponents_.size() - 1;

        auto [pageIndex, offset] = Locate(entity);

        //Resize only if the pageIndex is greater than the current size of sparse_
        if (pageIndex >= sparse_.size())
        {
            sparse_.resize(pageIndex + 1);
        }

        // Allocate a new page if it doesn't exist
        if (sparse_[pageIndex] == nullptr)
        {
            sparse_[pageIndex] = std::make_unique<Page>();
        }

        (*sparse_[pageIndex])[offset] = denseIndex;
    }

    template <typename Component>
    bool SparseSet<Component>::Contains(Entity entity) const
    {
        return Find(entity) != nullptr;
    }

    template <typename Component>
    Component& SparseSet<Component>::Get(Entity entity)
    {
        auto* componentPtr = Find(entity);
        assert(componentPtr);
        return *componentPtr;
    }

    template <typename Component>
    const Component& SparseSet<Component>::Get(Entity entity) const
    {
        assert(Contains(entity));

        auto [pageIndex, offset] = Locate(entity);

        return denseComponents_[(*sparse_[pageIndex])[offset]];
    }

    template <typename Component>
    Component* SparseSet<Component>::Find(Entity entity)
    {
        auto [pageIndex, offset] = Locate(entity);

        if (pageIndex >= sparse_.size() || sparse_[pageIndex] == nullptr)
            return nullptr;

        std::size_t denseIndex = (*sparse_[pageIndex])[offset];
        if (denseIndex >= denseEntities_.size())
            return nullptr;

        return denseEntities_[denseIndex] == entity
            ? &denseComponents_[denseIndex]
            : nullptr;
    }

    template <typename Component>
    const Component* SparseSet<Component>::Find(Entity entity) const
    {
        auto [pageIndex, offset] = Locate(entity);

        if (pageIndex >= sparse_.size() || sparse_[pageIndex] == nullptr)
            return nullptr;

        std::size_t denseIndex = (*sparse_[pageIndex])[offset];
        if (denseIndex >= denseEntities_.size())
            return nullptr;

        return denseEntities_[denseIndex] == entity
            ? &denseComponents_[denseIndex]
            : nullptr;
    }

    template <typename Component>
    Component& SparseSet<Component>::GetByDenseIndex(std::size_t index)
    {
        assert(index < denseComponents_.size());

        return denseComponents_[index];
    }

    template <typename Component>
    const Component& SparseSet<Component>::GetByDenseIndex(
        std::size_t index) const
    {
        assert(index < denseComponents_.size());

        return denseComponents_[index];
    }

    template <typename Component>
    void SparseSet<Component>::EntityDestroyed(Entity entity)
    {
        if (Contains(entity))
            Remove(entity);
    }

    template <typename Component>
    std::size_t SparseSet<Component>::Size() const
    {
        return denseComponents_.size();
    }

    template <typename Component>
    std::span<const Entity> SparseSet<Component>::GetEntities() const
    {
        return { denseEntities_.data(),
                 denseEntities_.size() };
    }
}
