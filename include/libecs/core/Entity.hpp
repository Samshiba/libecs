//
// Created by genin on 03/06/2026.
// Path: include/libecs/core/Entity.hpp
//

#pragma once

#include <cstdint>

namespace libecs::core
{
    using Entity = std::uint32_t;

    // Upper 24 bits for index
    inline constexpr std::uint32_t ENTITY_INDEX_MASK = 0xFFFFFF00;

    // Lower 8 bits for version
    inline constexpr std::uint32_t ENTITY_VERSION_MASK = 0x000000FF;

    // Reserved index (end of the free list), never given to a live entity
    inline constexpr std::uint32_t NULL_INDEX = 0xFFFFFF;

    inline constexpr Entity NULL_ENTITY = 0xFFFFFFFF;

    constexpr std::uint32_t GetEntityIndex(Entity entity)
    {
        return (ENTITY_INDEX_MASK & entity) >> 8;
    }

    constexpr std::uint32_t GetEntityVersion(Entity entity)
    {
        return ENTITY_VERSION_MASK & entity;
    }

    constexpr Entity CreateEntity(std::uint32_t index, std::uint32_t version)
    {
        return ((index << 8) & ENTITY_INDEX_MASK) | (version &
            ENTITY_VERSION_MASK);
    }
}
