//
// Created by genin on 04/06/2026.
// Path: include/libecs/core/registry/IPool.hpp
//

#pragma once

#include <libecs/core/Entity.hpp>

namespace libecs::core::registry
{
    class IPool
    {
    public:
        virtual ~IPool() = default;

        virtual void EntityDestroyed(Entity entity) = 0;
    };
}