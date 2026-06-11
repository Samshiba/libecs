//
// Created by genin on 04/06/2026.
// Path: include/libecs/core/registry/TypeId.hpp
//

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace libecs::core::registry
{
    using ComponentTypeId = uint32_t;

    inline ComponentTypeId GenerateNextTypeId()
    {
        static std::atomic<ComponentTypeId> id{ 0 };
        return id++;
    }

    template <typename T>
    ComponentTypeId GetTypeId()
    {
        static const ComponentTypeId id = GenerateNextTypeId();
        return id;
    }
}