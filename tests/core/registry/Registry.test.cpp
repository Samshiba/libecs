//
// Created by genin on 06/06/2026.
// Path: tests/core/registry/Registry.test.cpp
//

#include  <doctest.h>
#include <libecs/core/registry/Registry.hpp>

namespace
{
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
}

TEST_SUITE("Registry test suite")
{
    TEST_CASE("Entity Creation")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();

        CHECK_EQ(libecs::core::GetEntityIndex(entity1), 0u);
        CHECK_EQ(libecs::core::GetEntityVersion(entity1), 0u);

        CHECK_EQ(libecs::core::GetEntityIndex(entity2), 1u);
        CHECK_EQ(libecs::core::GetEntityVersion(entity2), 0u);

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

        CHECK_EQ(libecs::core::GetEntityIndex(entity5), 2u);
        CHECK_EQ(libecs::core::GetEntityVersion(entity5), 1u);

        CHECK_EQ(libecs::core::GetEntityIndex(entity6), 1u);
        CHECK_EQ(libecs::core::GetEntityVersion(entity6), 1u);
    }

    TEST_CASE("Destroy with components")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();

        registry.EmplaceComponent<PositionComponent>(entity1, 10.0f, 20.0f);

        CHECK(registry.HasComponent<PositionComponent>(entity1));

        CHECK(registry.DestroyEntity(entity1));
        CHECK(!registry.IsEntityValid(entity1));

        libecs::core::Entity entity2 = registry.CreateEntity();
        CHECK(!registry.HasComponent<PositionComponent>(entity2));
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

    TEST_CASE("Replacing a component")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();

        registry.EmplaceComponent<PositionComponent>(entity1, 10.0f, 20.0f);
        CHECK(registry.EmplaceComponent<PositionComponent>(entity1, 30.0f, 40.0f
              ) ==
              PositionComponent{30.0f, 40.0f});
        CHECK(registry.EmplaceComponent<PositionComponent>(entity2, 50.0f, 60.0f
              ) ==
              PositionComponent{50.0f, 60.0f});

        CHECK(registry.HasComponent<PositionComponent>(entity1));

        registry.RemoveComponent<PositionComponent>(entity1);
        CHECK(!registry.HasComponent<PositionComponent>(entity1));
        CHECK(registry.HasComponent<PositionComponent>(entity2));
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

        CHECK(registry.HasComponent<PositionComponent>(entity1));
    }

    TEST_CASE("Getting components")
    {
        auto registry = libecs::core::registry::Registry();

        libecs::core::Entity entity1 = registry.CreateEntity();
        libecs::core::Entity entity2 = registry.CreateEntity();

        registry.EmplaceComponent<PositionComponent>(entity1, 10.0f, 20.0f);

        CHECK(registry.GetComponent<PositionComponent>(entity1) ==
              PositionComponent{10.0f, 20.0f});

        registry.GetComponent<PositionComponent>(entity1).x = 99.0f;
        CHECK(registry.GetComponent<PositionComponent>(entity1).x == 99.0f);

        CHECK(registry.TryGetComponent<PositionComponent>(entity1) != nullptr);
        CHECK(registry.TryGetComponent<PositionComponent>(entity2) == nullptr);
        CHECK(registry.TryGetComponent<int>(entity1) == nullptr);

        const auto& constRegistry = registry;
        CHECK(constRegistry.GetComponent<PositionComponent>(entity1).x == 99.0f);
        CHECK(constRegistry.TryGetComponent<int>(entity1) == nullptr);
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