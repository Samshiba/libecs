//
// Created by genin on 06/06/2026.
// Path: src/core/Registry.cpp
//

#include <libecs/core/registry/Registry.hpp>

namespace libecs::core::registry
{
    Registry::Registry()
    {
        entities_.reserve(4096);
    }

    Entity Registry::CreateEntity()
    {
        if (nextFreeIndex_ == NULL_INDEX)
        {
            Entity entity = core::CreateEntity(
                static_cast<uint32_t>(entities_.size()), 0);
            entities_.push_back(entity);
            return entity;
        }

        Entity deadEntity = entities_[nextFreeIndex_];
        Entity newEntity = core::CreateEntity(nextFreeIndex_,
                                              GetEntityVersion(deadEntity) + 1);
        entities_[nextFreeIndex_] = newEntity;
        nextFreeIndex_ = GetEntityIndex(deadEntity);
        return newEntity;
    }

    bool Registry::DestroyEntity(Entity entity)
    {
        std::uint32_t index = GetEntityIndex(entity);

        if (index >= entities_.size() || entities_[index] != entity)
        {
            return false;
        }

        Entity deadEntity = core::CreateEntity(nextFreeIndex_,
                                               GetEntityVersion(entity));

        entities_[index] = deadEntity;
        nextFreeIndex_ = index;

        // TODO destroy components associated with this entity
        return true;
    }

    bool Registry::IsEntityValid(Entity entity) const
    {
        uint32_t entityIndex = GetEntityIndex(entity);
        return entityIndex < entities_.size() && entities_[entityIndex] ==
            entity;
    }
}