//
// Created by genin on 03/06/2026.
// Path: include/libecs/core/entity.hpp
//

#pragma once

#include <cstdint>

namespace libecs::core
{
    using Entity = uint32_t;
    constexpr Entity INVALID_ENTITY = 0;

    std::uint32_t GetEntityIndex(Entity entity);
    std::uint32_t GetEntityVersion(Entity entity);

    Entity CreateEntity(std::uint32_t index, std::uint32_t version);

    // Upper 24 bits for index
    inline constexpr std::uint32_t ENTITY_INDEX_MASK = 0xFFFFFF00;

    // Lower 8 bits for version
    inline constexpr std::uint32_t ENTITY_VERSION_MASK = 0x000000FF;

    inline std::uint32_t core::GetEntityIndex(Entity entity)
    {
        return (ENTITY_INDEX_MASK & entity) >> 8;
    }

    inline std::uint32_t core::GetEntityVersion(Entity entity)
    {
        return ENTITY_VERSION_MASK & entity;
    }

    inline Entity core::CreateEntity(std::uint32_t index,
                                     std::uint32_t version)
    {
        return ((index << 8) & ENTITY_INDEX_MASK) | (version &
            ENTITY_VERSION_MASK);
    }
}


