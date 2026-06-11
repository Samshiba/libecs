//
// Created by genin on 04/06/2026.
// Path: tests/core/registry/TypeId.test.cpp
//

#include <doctest.h>
#include <libecs/core/registry/TypeId.hpp>

TEST_SUITE("TypeId Generation")
{
    TEST_CASE("Unique TypeIds for different types")
    {
        auto idInt = libecs::core::registry::GetTypeId<int>();
        auto idFloat = libecs::core::registry::GetTypeId<float>();
        auto idDouble = libecs::core::registry::GetTypeId<double>();

        CHECK(idInt != idFloat);
        CHECK(idInt != idDouble);
        CHECK(idFloat != idDouble);
    }

    TEST_CASE("Consistent TypeIds for the same type")
    {
        auto idInt1 = libecs::core::registry::GetTypeId<int>();
        auto idFloat = libecs::core::registry::GetTypeId<float>();
        auto idInt2 = libecs::core::registry::GetTypeId<int>();

        CHECK(idFloat != idInt1);
        CHECK(idFloat != idInt2);
        CHECK(idInt1 == idInt2);
    }
}