//
// Created by genin on 04/06/2026.
// Path: tests/core/SparseSet.test.cpp
//

#include <doctest.h>
#include <libecs/core/SparseSet.hpp>

namespace
{
    struct PositionComponent
    {
        float x;
        float y;
        bool operator==(const PositionComponent&) const = default;
    };
}

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

    TEST_CASE("Insertion with varying entity indices")
    {
        libecs::core::SparseSet<int> ints;
        libecs::core::Entity e_low = libecs::core::CreateEntity(5, 0);
        libecs::core::Entity e_high = libecs::core::CreateEntity(100000, 0);
        libecs::core::Entity e_medium = libecs::core::CreateEntity(5000, 0);

        ints.Insert(e_low, 100);
        ints.Insert(e_high, 200);
        ints.Insert(e_medium, 300);

        CHECK(ints.Contains(e_low) == true);
        CHECK(ints.Contains(e_high) == true);
        CHECK(ints.Contains(e_medium) == true);

        CHECK(ints.Get(e_medium) == 300);
    }

    TEST_CASE("Inserting multiple components for the same entity")
    {
        libecs::core::SparseSet<int> ints;
        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);

        ints.Insert(e1, 100);
        CHECK(ints.Get(e1) == 100);

        // Insert again with a different value
        ints.Insert(e1, 200);
        CHECK(ints.Get(e1) == 200);

        ints.Insert(e2, 300);
        ints.Remove(e1);
        CHECK(ints.Contains(e1) == false);
        CHECK(ints.Get(e2) == 300);
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

    TEST_CASE("Get entities")
    {
        libecs::core::SparseSet<int> ints;
        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);
        libecs::core::Entity e3 = libecs::core::CreateEntity(3, 0);

        ints.Insert(e1, 100);
        ints.Insert(e2, 200);
        ints.Insert(e3, 300);

        auto entities = ints.GetEntities();
        CHECK(entities.size() == 3);
        CHECK(entities[0] == e1);
        CHECK(entities[1] == e2);
        CHECK(entities[2] == e3);
    }

    TEST_CASE("Find")
    {
        libecs::core::SparseSet<int> ints;
        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);

        ints.Insert(e1, 100);

        SUBCASE("present entity returns its component")
        {
            int* value = ints.Find(e1);
            REQUIRE(value != nullptr);
            CHECK(*value == 100);

            *value = 150;
            CHECK(ints.Get(e1) == 150);
        }

        SUBCASE("absent entity in an existing page returns nullptr")
        {
            CHECK(ints.Find(e2) == nullptr);
        }

        SUBCASE("entity in a page that doesn't exist returns nullptr")
        {
            CHECK(ints.Find(libecs::core::CreateEntity(100000, 0)) == nullptr);
        }

        SUBCASE("stale handle (same index, other version) returns nullptr")
        {
            CHECK(ints.Find(libecs::core::CreateEntity(1, 1)) == nullptr);
        }

        SUBCASE("removed entity returns nullptr")
        {
            ints.Remove(e1);
            CHECK(ints.Find(e1) == nullptr);
        }

        SUBCASE("const version")
        {
            const auto& constInts = ints;
            REQUIRE(constInts.Find(e1) != nullptr);
            CHECK(*constInts.Find(e1) == 100);
            CHECK(constInts.Find(e2) == nullptr);
        }
    }

    TEST_CASE("GetByDenseIndex matches GetEntities order")
    {
        libecs::core::SparseSet<int> ints;
        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);
        libecs::core::Entity e3 = libecs::core::CreateEntity(3, 0);

        ints.Insert(e1, 100);
        ints.Insert(e2, 200);
        ints.Insert(e3, 300);

        CHECK(ints.GetByDenseIndex(0) == 100);
        CHECK(ints.GetByDenseIndex(1) == 200);
        CHECK(ints.GetByDenseIndex(2) == 300);

        // Swap and pop: e3 (last) moves into e1's slot
        ints.Remove(e1);

        REQUIRE(ints.Size() == 2);
        CHECK(ints.GetEntities()[0] == e3);
        CHECK(ints.GetByDenseIndex(0) == 300);
        CHECK(ints.GetByDenseIndex(1) == 200);

        // Dense index and entity lookups reach the same component
        for (std::size_t i = 0; i < ints.Size(); ++i)
        {
            CHECK(&ints.GetByDenseIndex(i) ==
                &ints.Get(ints.GetEntities()[i]));
        }

        ints.GetByDenseIndex(1) = 250;
        CHECK(ints.Get(e2) == 250);
    }
}
