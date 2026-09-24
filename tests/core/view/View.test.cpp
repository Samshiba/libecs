//
// Created by genin on 23/09/2026.
// Path: tests/core/view/View.test.cpp
//

#include <doctest.h>
#include <map>
#include <vector>

#include <libecs/core/registry/Registry.hpp>
#include <libecs/core/view/View.hpp>

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

    struct VelocityComponent
    {
        float vx;
        float vy;

        VelocityComponent(float vx, float vy)
            : vx(vx), vy(vy)
        {
        }

        bool operator==(const VelocityComponent&) const = default;
    };
}

TEST_SUITE("View Test Suite")
{
    TEST_CASE("View Each and Contains")
    {
        libecs::core::SparseSet<PositionComponent> positions;
        libecs::core::SparseSet<VelocityComponent> velocities;

        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);

        positions.Insert(e1, { 10.0f, 20.0f });
        positions.Insert(e2, { 30.0f, 40.0f });

        velocities.Insert(e1, { 1.0f, 2.0f });

        libecs::core::view::View<PositionComponent, VelocityComponent> view(
            &positions, &velocities);

        CHECK(view.Contains(e1) == true);
        CHECK(view.Contains(e2) == false);

        std::vector<libecs::core::Entity> visitedEntities;
        view.Each([&visitedEntities](libecs::core::Entity entity,
                                     PositionComponent& pos,
                                     VelocityComponent& vel) {
            visitedEntities.push_back(entity);
            pos.x += vel.vx;
            pos.y += vel.vy;
        });

        CHECK(visitedEntities.size() == 1);
        CHECK(visitedEntities[0] == e1);
        CHECK(positions.Get(e1) == PositionComponent{11.0f, 22.0f});
    }

    TEST_CASE("View MaxSize")
    {
        libecs::core::SparseSet<PositionComponent> positions;
        libecs::core::SparseSet<VelocityComponent> velocities;

        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);

        positions.Insert(e1, { 10.0f, 20.0f });
        positions.Insert(e2, { 30.0f, 40.0f });

        velocities.Insert(e1, { 1.0f, 2.0f });

        libecs::core::view::View<PositionComponent, VelocityComponent> view(
            &positions, &velocities);
        CHECK(view.MaxSize() == 1);

        libecs::core::view::View<PositionComponent> view2(&positions);
        CHECK(view2.MaxSize() == 2);
    }

    TEST_CASE("View Get")
    {
        libecs::core::SparseSet<PositionComponent> positions;

        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);

        positions.Insert(e1, { 10.0f, 20.0f });

        libecs::core::view::View<PositionComponent> view(&positions);
        CHECK(view.Get<PositionComponent>(e1) == PositionComponent{10.0f,
              20.0f});

        view.Get<PositionComponent>(e1).x = 300.0f;
        CHECK(view.Get<PositionComponent>(e1) == PositionComponent{300.0f,
              20.0f});
    }

    TEST_CASE("All but with nullptr")
    {
        libecs::core::SparseSet<PositionComponent> positions;
        libecs::core::SparseSet<VelocityComponent> velocities;

        libecs::core::Entity e1 = libecs::core::CreateEntity(1, 0);
        libecs::core::Entity e2 = libecs::core::CreateEntity(2, 0);

        positions.Insert(e1, { 10.0f, 20.0f });
        positions.Insert(e2, { 30.0f, 40.0f });

        velocities.Insert(e1, { 1.0f, 2.0f });

        libecs::core::view::View<PositionComponent, VelocityComponent> view(
            &positions, nullptr);

        CHECK(view.Contains(e1) == false);
        CHECK(view.Contains(e2) == false);

        CHECK(view.MaxSize() == 0);

        std::vector<libecs::core::Entity> visitedEntities;
        view.Each([&visitedEntities](libecs::core::Entity entity,
                                     PositionComponent& pos,
                                     VelocityComponent& vel) {
            visitedEntities.push_back(entity);
            pos.x += vel.vx;
            pos.y += vel.vy;
        });

        CHECK(visitedEntities.size() == 0);
    }
}

namespace
{
    using libecs::core::Entity;
    using libecs::core::registry::Registry;

    constexpr std::size_t ENTITY_COUNT = 500;

    // Every entity gets a Position, one out of `velocityEvery` gets a Velocity
    std::vector<Entity> CreateEntities(Registry& registry,
                                       std::size_t velocityEvery = 1)
    {
        std::vector<Entity> entities;

        for (std::size_t i = 0; i < ENTITY_COUNT; ++i)
        {
            const Entity entity = registry.CreateEntity();
            registry.EmplaceComponent<PositionComponent>(entity, 0.0f, 0.0f);

            if (i % velocityEvery == 0)
            {
                registry.EmplaceComponent<VelocityComponent>(entity, 1.0f,
                    1.0f);
            }

            entities.push_back(entity);
        }

        return entities;
    }

    // Checks that each expected entity was visited exactly once
    void CheckVisitedOnce(const std::map<Entity, int>& visits,
                          const std::vector<Entity>& expected)
    {
        CHECK(visits.size() == expected.size());

        for (const Entity entity : expected)
        {
            const auto it = visits.find(entity);
            REQUIRE(it != visits.end());
            CHECK(it->second == 1);
        }
    }
}

TEST_SUITE("View modification during Each")
{
    // Only the current entity may be modified inside Each
    // (see Limitations in the README)

    TEST_CASE("Destroying the current entity visits every entity once")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        // Dense order == creation order: first, middle and last positions
        // exercise the swap-and-pop edge cases
        Entity toDestroy = libecs::core::NULL_ENTITY;

        SUBCASE("first in the dense array")
        {
            toDestroy = entities.front();
        }
        SUBCASE("middle of the dense array")
        {
            toDestroy = entities[ENTITY_COUNT / 2];
        }
        SUBCASE("last in the dense array")
        {
            toDestroy = entities.back();
        }

        std::map<Entity, int> visits;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                ++visits[entity];

                if (entity == toDestroy)
                {
                    registry.DestroyEntity(entity);
                }
            });

        CheckVisitedOnce(visits, entities);
        CHECK(!registry.IsEntityValid(toDestroy));
    }

    TEST_CASE("Destroying many entities visits every entity once")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        std::vector<Entity> destroyed;
        std::map<Entity, int> visits;

        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                ++visits[entity];

                if (libecs::core::GetEntityIndex(entity) % 7 == 0)
                {
                    registry.DestroyEntity(entity);
                    destroyed.push_back(entity);
                }
            });

        CheckVisitedOnce(visits, entities);

        for (const Entity entity : destroyed)
        {
            CHECK(!registry.IsEntityValid(entity));
        }

        // The survivors are still all in the view, with their components
        std::size_t remaining = 0;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity, PositionComponent&, VelocityComponent&) {
                ++remaining;
            });
        CHECK(remaining == ENTITY_COUNT - destroyed.size());
    }

    TEST_CASE("Destroying every entity visits every entity once")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        std::map<Entity, int> visits;
        auto view = registry.GetView<PositionComponent, VelocityComponent>();

        view.Each([&](Entity entity, PositionComponent&, VelocityComponent&) {
            ++visits[entity];
            registry.DestroyEntity(entity);
        });

        CheckVisitedOnce(visits, entities);
        CHECK(view.MaxSize() == 0);
    }

    TEST_CASE("Removing the current entity's component from the iterated pool")
    {
        Registry registry;

        // Half of the entities have a Velocity: the Velocity pool is the
        // smallest one, so it is the pool being iterated
        CreateEntities(registry, 2);

        std::vector<Entity> expected;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                expected.push_back(entity);
            });
        REQUIRE(expected.size() == ENTITY_COUNT / 2);

        std::map<Entity, int> visits;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                ++visits[entity];
                registry.RemoveComponent<VelocityComponent>(entity);
            });

        CheckVisitedOnce(visits, expected);

        for (const Entity entity : expected)
        {
            CHECK(!registry.HasComponent<VelocityComponent>(entity));
            CHECK(registry.HasComponent<PositionComponent>(entity));
        }
    }

    TEST_CASE("Removing the current entity's component from another pool")
    {
        Registry registry;

        // The Velocity pool is iterated, Position components get removed
        CreateEntities(registry, 2);

        std::vector<Entity> expected;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                expected.push_back(entity);
            });

        std::map<Entity, int> visits;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                ++visits[entity];
                registry.RemoveComponent<PositionComponent>(entity);
            });

        CheckVisitedOnce(visits, expected);

        for (const Entity entity : expected)
        {
            CHECK(!registry.HasComponent<PositionComponent>(entity));
        }
    }

    TEST_CASE("Components modified in Each keep the right values")
    {
        // Swap-and-pop moves components around: make sure every survivor
        // still gets the update meant for it
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent& position,
                VelocityComponent& velocity) {
                if (libecs::core::GetEntityIndex(entity) % 3 == 0)
                {
                    registry.DestroyEntity(entity);
                    return;
                }

                position.x += velocity.vx;
            });

        for (const Entity entity : entities)
        {
            if (registry.IsEntityValid(entity))
            {
                CHECK(registry.GetComponent<PositionComponent>(entity).x ==
                    1.0f);
            }
        }
    }
}
