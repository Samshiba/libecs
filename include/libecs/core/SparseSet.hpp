//
// Created by genin on 03/06/2026.
// Path: include/libecs/core/SparseSet.hpp
//

#pragma once

#include <vector>
#include <array>
#include <memory>
#include <cassert>

#include "Entity.hpp"

namespace libecs::core
{
    template <typename Component>
    class SparseSet
    {
    public:
        SparseSet() = default;
        ~SparseSet() = default;

        void Insert(Entity entity, Component component);
        void Remove(Entity entity);

        [[nodiscard]] bool Contains(Entity entity) const;
        Component& Get(Entity entity);

    private:
        std::vector<Component> denseComponents_;
        std::vector<Entity> denseEntities_;
        std::vector<std::unique_ptr<std::array<std::size_t, 4096> > > sparse_;
    };

    template <typename Component>
    void SparseSet<Component>::Insert(Entity entity, Component component)
    {
        denseComponents_.push_back(component);
        denseEntities_.push_back(entity);
        std::size_t denseIndex = denseComponents_.size() - 1;

        uint32_t entityIndex = GetEntityIndex(entity);
        std::size_t pageIndex = entityIndex / 4096;
        std::size_t offset = entityIndex % 4096;

        sparse_.resize(pageIndex + 1);
        if (sparse_[pageIndex] == nullptr)
        {
            sparse_[pageIndex] = std::make_unique<std::array<size_t, 4096> >();
        }

        (*sparse_[pageIndex])[offset] = denseIndex;
    }

    template <typename Component>
    void SparseSet<Component>::Remove(Entity entity)
    {
        uint32_t entityIndex = GetEntityIndex(entity);
        std::size_t pageIndex = entityIndex / 4096;
        std::size_t offset = entityIndex % 4096;
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
            std::size_t lastEntityIndex = GetEntityIndex(lastEntity);
            std::size_t lastPageIndex = lastEntityIndex / 4096;
            std::size_t lastOffset = lastEntityIndex % 4096;
            (*sparse_[lastPageIndex])[lastOffset] = denseIndex;
        }
    }

    template <typename Component>
    bool SparseSet<Component>::Contains(Entity entity) const
    {
        uint32_t entityIndex = GetEntityIndex(entity);
        std::size_t pageIndex = entityIndex / 4096;
        std::size_t offset = entityIndex % 4096;

        if (pageIndex >= sparse_.size() || sparse_[pageIndex] == nullptr)
            return false;

        std::size_t denseIndex = (*sparse_[pageIndex])[offset];
        if (denseIndex >= denseEntities_.size())
            return false;

        return denseEntities_[denseIndex] == entity;
    }

    template <typename Component>
    Component& SparseSet<Component>::Get(Entity entity)
    {
#ifndef NODEBUG
        assert(Contains(entity));
#endif

        uint32_t entityIndex = GetEntityIndex(entity);
        std::size_t pageIndex = entityIndex / 4096;
        std::size_t offset = entityIndex % 4096;

        return denseComponents_[(*sparse_[pageIndex])[offset]];
    }
}

