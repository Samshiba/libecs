//
// Created by genin on 04/06/2026.
// Path: tests/core/SparseSet.test.cpp
//

#include <doctest.h>
#include <libecs/core/SparseSet.hpp>

struct PositionComponent
{
    float x;
    float y;
    bool operator==(const PositionComponent&) const = default;
};

TEST_SUITE("SparseSet Core Mechanics")
{
    TEST_CASE("Insertion and basic retrieval")
    {
        libecs::core::SparseSet<PositionComponent> positions;
        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);

        positions.Insert(e1, { 10.0f, 20.0f });
        positions.Insert(e2, { 30.0f, 40.0f });

        CHECK(positions.Contains(e1) == true);
        CHECK(positions.Contains(e2) == true);

        libecs::core::Entity e3 = libecs::core::CreateEntity(3, 0);
        CHECK(positions.Contains(e3) == false);

        CHECK(positions.Get(e1) == PositionComponent{10.0f, 20.0f});
        CHECK(positions.Get(e2) == PositionComponent{30.0f, 40.0f});

        positions.Get(e1).x = 99.0f;
        CHECK(positions.Get(e1) == PositionComponent{99.0f, 20.0f});
    }

    TEST_CASE("Swap and Pop logic (Removal)")
    {
        libecs::core::SparseSet<int> ints;
        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);
        libecs::core::Entity e3 = libecs::core::CreateEntity(3, 0);

        ints.Insert(e1, 100);
        ints.Insert(e2, 200);
        ints.Insert(e3, 300);

        // Delete middle element
        ints.Remove(e2);

        CHECK(ints.Contains(e2) == false);
        CHECK(ints.Contains(e1) == true);
        CHECK(ints.Contains(e3) == true);

        CHECK(ints.Get(e1) == 100);
        CHECK(ints.Get(e3) == 300);

        // Delete last, edge case of removing the last element
        ints.Remove(e3);
        CHECK(ints.Contains(e3) == false);
        CHECK(ints.Contains(e1) == true);
        CHECK(ints.Get(e1) == 100);
    }

    TEST_CASE("Pagination boundary limits")
    {
        libecs::core::SparseSet<int> ints;

        // Page 0
        libecs::core::Entity e_low = libecs::core::CreateEntity(5, 0);

        // Page 1 (4096 / 4096 = 1)
        libecs::core::Entity e_high = libecs::core::CreateEntity(5000, 0);

        // Page 24 (100000 / 4096 = 24)
        libecs::core::Entity e_massive = libecs::core::CreateEntity(100000, 0);

        ints.Insert(e_low, 42);
        ints.Insert(e_high, 84);
        ints.Insert(e_massive, 999);

        // Check pages creation
        CHECK(ints.Contains(e_low) == true);
        CHECK(ints.Contains(e_high) == true);
        CHECK(ints.Contains(e_massive) == true);

        CHECK(ints.Get(e_low) == 42);
        CHECK(ints.Get(e_high) == 84);
        CHECK(ints.Get(e_massive) == 999);

        ints.Remove(e_high);
        CHECK(ints.Contains(e_high) == false);
        CHECK(ints.Contains(e_massive) == true);
    }
}