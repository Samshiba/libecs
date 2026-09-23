//
// Created by genin on 06/06/2026.
// Path: tests/core/registry/Registry.test.cpp
//

#include  <doctest.h>
#include <libecs/core/registry/Registry.hpp>

struct PositionComponent
{
    float x;
    float y;

    PositionComponent(float x, float y)
        : x(x), y(y)
    {
    }

    bool operator==(const PositionComponent&) const = default;
};

TEST_SUITE("Registry test suite")
{
    TEST_CASE("Entity Creation")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();

        CHECK_EQ(libecs::core::GetEntityIndex(entity1), 0);
        CHECK_EQ(libecs::core::GetEntityVersion(entity1), 0);

        CHECK_EQ(libecs::core::GetEntityIndex(entity2), 1);
        CHECK_EQ(libecs::core::GetEntityVersion(entity2), 0);

        CHECK(registry.IsEntityValid(entity1));
        CHECK(registry.IsEntityValid(entity2));
    }

    TEST_CASE("Entity Deleted")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();
        libecs::core::Entity entity3 = registry.CreateEntity();
        libecs::core::Entity entity4 = registry.CreateEntity();

        CHECK(registry.IsEntityValid(entity1));
        CHECK(registry.IsEntityValid(entity2));
        CHECK(registry.IsEntityValid(entity3));
        CHECK(registry.IsEntityValid(entity4));

        CHECK(registry.DestroyEntity(entity2));
        CHECK(registry.DestroyEntity(entity3));

        CHECK(!registry.IsEntityValid(entity2));
        CHECK(!registry.IsEntityValid(entity3));

        libecs::core::Entity entity5 = registry.CreateEntity();
        libecs::core::Entity entity6 = registry.CreateEntity();

        CHECK_EQ(libecs::core::GetEntityIndex(entity5), 2);
        CHECK_EQ(libecs::core::GetEntityVersion(entity5), 1);

        CHECK_EQ(libecs::core::GetEntityIndex(entity6), 1);
        CHECK_EQ(libecs::core::GetEntityVersion(entity6), 1);
    }

    TEST_CASE("Component")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();

        CHECK(registry.EmplaceComponent<PositionComponent>(entity1, 10.0f, 20.0f
              ) ==
              PositionComponent{10.0f, 20.0f});
        CHECK(registry.EmplaceComponent<PositionComponent>(entity2, 30.0f, 40.0f
              ) ==
              PositionComponent{30.0f, 40.0f});

        CHECK(registry.HasComponent<PositionComponent>(entity1));
        CHECK(registry.HasComponent<PositionComponent>(entity2));

        registry.RemoveComponent<PositionComponent>(entity1);
        CHECK(!registry.HasComponent<PositionComponent>(entity1));
    }

    TEST_CASE("Removing uncreated component type")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();

        CHECK(registry.EmplaceComponent<PositionComponent>(entity1, 10.0f, 20.0f
              ) ==
              PositionComponent{10.0f, 20.0f});

        CHECK(registry.HasComponent<PositionComponent>(entity1));

        registry.RemoveComponent<int>(entity1);
    }

    TEST_CASE("Removing unexistent component")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();

        CHECK(registry.EmplaceComponent<PositionComponent>(entity1, 10.0f, 20.0f
              ) ==
              PositionComponent{10.0f, 20.0f});

        CHECK(registry.HasComponent<PositionComponent>(entity1));
        CHECK(!registry.HasComponent<PositionComponent>(entity2));

        registry.RemoveComponent<PositionComponent>(entity2);

        CHECK(registry.HasComponent<PositionComponent>(entity1));
        CHECK(!registry.HasComponent<PositionComponent>(entity2));
    }
}