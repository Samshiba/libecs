//
// Created by genin on 03/06/2026.
// Path: tests/core/entity.cpp
//

#include <doctest.h>
#include <libecs/core/Entity.hpp>

TEST_CASE("Entity Creation")
{
    auto entity = libecs::core::CreateEntity(10, 5);
    CHECK(libecs::core::GetEntityIndex(entity) == 10);
    CHECK(libecs::core::GetEntityVersion(entity) == 5);
}