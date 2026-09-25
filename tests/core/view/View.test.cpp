//
// Created by genin on 23/09/2026.
// Path: tests/core/view/View.test.cpp
//

#include <doctest.h>
#include <iterator>
#include <map>
#include <type_traits>
#include <ranges>
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


namespace
{
    struct HealthComponent
    {
        int value;

        explicit HealthComponent(int value)
            : value(value)
        {
        }
    };
}

TEST_SUITE("View pivot selection")
{
    // Each() turns the runtime index of the smallest pool into a template
    // parameter: check that every possible pivot position works
    TEST_CASE("Three components, the smallest pool at every position")
    {
        Registry registry;

        // Pool sizes: the "small" component is on 1 entity out of 5, the
        // others on 1 out of 2 and on every entity
        std::size_t positionEvery = 1;
        std::size_t velocityEvery = 1;
        std::size_t healthEvery = 1;

        SUBCASE("pivot is the first pool (Position)")
        {
            positionEvery = 5;
            velocityEvery = 2;
        }
        SUBCASE("pivot is the second pool (Velocity)")
        {
            velocityEvery = 5;
            healthEvery = 2;
        }
        SUBCASE("pivot is the third pool (Health)")
        {
            healthEvery = 5;
            positionEvery = 2;
        }

        std::vector<Entity> expected;

        for (std::size_t i = 0; i < ENTITY_COUNT; ++i)
        {
            const Entity entity = registry.CreateEntity();
            const bool hasPosition = i % positionEvery == 0;
            const bool hasVelocity = i % velocityEvery == 0;
            const bool hasHealth = i % healthEvery == 0;

            if (hasPosition)
                registry.EmplaceComponent<PositionComponent>(entity, 0.0f,
                    0.0f);
            if (hasVelocity)
                registry.EmplaceComponent<VelocityComponent>(entity, 1.0f,
                    2.0f);
            if (hasHealth)
                registry.EmplaceComponent<HealthComponent>(entity,
                    static_cast<int>(i));

            if (hasPosition && hasVelocity && hasHealth)
                expected.push_back(entity);
        }

        REQUIRE(!expected.empty());

        std::map<Entity, int> visits;
        registry.GetView<PositionComponent, VelocityComponent,
                         HealthComponent>().Each(
            [&](Entity entity, PositionComponent& position,
                VelocityComponent& velocity, HealthComponent& health) {
                ++visits[entity];

                // Each reference must belong to the visited entity
                CHECK(health.value == static_cast<int>(
                    libecs::core::GetEntityIndex(entity)));

                position.x += velocity.vx;
                health.value = -1;
            });

        CheckVisitedOnce(visits, expected);

        for (const Entity entity : expected)
        {
            CHECK(registry.GetComponent<PositionComponent>(entity).x == 1.0f);
            CHECK(registry.GetComponent<HealthComponent>(entity).value == -1);
        }
    }

    TEST_CASE("Single component view")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        std::map<Entity, int> visits;
        registry.GetView<PositionComponent>().Each(
            [&](Entity entity, PositionComponent& position) {
                ++visits[entity];
                position.y = 5.0f;
            });

        CheckVisitedOnce(visits, entities);
        CHECK(registry.GetComponent<PositionComponent>(entities.front()).y ==
            5.0f);
    }
}


namespace
{
    // True when GetView<Ts...>() compiles on a RegistryT (Registry or
    // const Registry): lets the tests check what must NOT compile
    template <typename RegistryT, typename... Ts>
    concept CanGetView = requires(RegistryT& registry)
    {
        registry.template GetView<Ts...>();
    };
}

TEST_SUITE("Read-only views")
{
    // A registry can give any view, a const registry only read-only ones
    static_assert(CanGetView<Registry, PositionComponent>);
    static_assert(CanGetView<Registry, const PositionComponent>);
    static_assert(CanGetView<const Registry, const PositionComponent>);
    static_assert(CanGetView<const Registry, const PositionComponent,
                             const VelocityComponent>);
    static_assert(!CanGetView<const Registry, PositionComponent>);
    static_assert(!CanGetView<const Registry, const PositionComponent,
                              VelocityComponent>);

    TEST_CASE("A const view visits the same entities as a mutable one")
    {
        // If GetView forgot remove_const, the const pool would never be
        // found and this view would silently be empty
        Registry registry;
        CreateEntities(registry, 3);

        std::map<Entity, int> mutableVisits;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                ++mutableVisits[entity];
            });

        std::map<Entity, int> constVisits;
        registry.GetView<const PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, const PositionComponent&, VelocityComponent&) {
                ++constVisits[entity];
            });

        REQUIRE(!mutableVisits.empty());
        CHECK(constVisits == mutableVisits);
    }

    TEST_CASE("Const components are passed as const references")
    {
        Registry registry;
        CreateEntities(registry);

        std::size_t visits = 0;
        registry.GetView<const PositionComponent, VelocityComponent>().Each(
            [&](Entity, auto& position, auto& velocity) {
                static_assert(std::is_same_v<decltype(position),
                                             const PositionComponent&>);
                static_assert(std::is_same_v<decltype(velocity),
                                             VelocityComponent&>);

                // The non-const component stays writable
                velocity.vx = position.x + 5.0f;
                ++visits;
            });

        CHECK(visits == ENTITY_COUNT);

        registry.GetView<VelocityComponent>().Each(
            [](Entity, const VelocityComponent& velocity) {
                CHECK(velocity.vx == 5.0f);
            });
    }

    TEST_CASE("Get returns a const reference for a const component")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        auto view = registry.GetView<const PositionComponent,
                                     VelocityComponent>();
        const Entity entity = entities.front();

        static_assert(std::is_same_v<
            decltype(view.Get<const PositionComponent>(entity)),
            const PositionComponent&>);
        static_assert(std::is_same_v<
            decltype(view.Get<VelocityComponent>(entity)),
            VelocityComponent&>);

        CHECK(view.Get<const PositionComponent>(entity).x == 0.0f);

        view.Get<VelocityComponent>(entity).vx = 42.0f;
        CHECK(registry.GetComponent<VelocityComponent>(entity).vx == 42.0f);
    }

    TEST_CASE("Views on a const registry")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry, 2);
        const Registry& constRegistry = registry;

        std::map<Entity, int> visits;
        constRegistry.GetView<const PositionComponent,
                              const VelocityComponent>().Each(
            [&](Entity entity, const PositionComponent&,
                const VelocityComponent&) {
                ++visits[entity];
            });

        CHECK(visits.size() == ENTITY_COUNT / 2);
        CHECK(constRegistry.GetView<const PositionComponent,
                                    const VelocityComponent>().MaxSize() ==
            ENTITY_COUNT / 2);
    }

    TEST_CASE("Const view of a component type never added is empty")
    {
        Registry registry;
        CreateEntities(registry);
        const Registry& constRegistry = registry;

        // HealthComponent has no pool yet: const GetPool returns nullptr
        std::size_t visits = 0;
        constRegistry.GetView<const PositionComponent,
                              const HealthComponent>().Each(
            [&](Entity, const PositionComponent&, const HealthComponent&) {
                ++visits;
            });

        CHECK(visits == 0);
        CHECK(constRegistry.GetView<const HealthComponent>().MaxSize() == 0);
    }
}


TEST_SUITE("Range-based for over views")
{
    using MoveView = libecs::core::view::View<PositionComponent,
                                              VelocityComponent>;
    using ReadOnlyView = libecs::core::view::View<const PositionComponent,
                                                  VelocityComponent>;

    // What <ranges> requires: fails with the missing requirement if not met
    static_assert(std::input_iterator<MoveView::ViewIterator>);
    static_assert(std::sentinel_for<std::default_sentinel_t,
                                    MoveView::ViewIterator>);
    static_assert(std::ranges::input_range<MoveView>);
    static_assert(std::ranges::input_range<ReadOnlyView>);

    TEST_CASE("Visits the same entities as Each")
    {
        Registry registry;
        CreateEntities(registry, 3);

        std::map<Entity, int> eachVisits;
        registry.GetView<PositionComponent, VelocityComponent>().Each(
            [&](Entity entity, PositionComponent&, VelocityComponent&) {
                ++eachVisits[entity];
            });

        std::map<Entity, int> loopVisits;
        for (auto [entity, position, velocity] :
             registry.GetView<PositionComponent, VelocityComponent>())
        {
            ++loopVisits[entity];
        }

        REQUIRE(!eachVisits.empty());
        CHECK(loopVisits == eachVisits);
    }

    TEST_CASE("Skips entities of the iterated pool that don't match")
    {
        // Position is the smallest pool (3 vs 4), and its last entity has no
        // Velocity: begin() must skip it, and so must operator++ for others
        Registry registry;
        std::vector<Entity> entities;
        for (int i = 0; i < 5; ++i)
            entities.push_back(registry.CreateEntity());

        for (int i : { 0, 1, 2 })
            registry.EmplaceComponent<PositionComponent>(entities[i], 0.0f,
                0.0f);
        for (int i : { 0, 1, 3, 4 })
            registry.EmplaceComponent<VelocityComponent>(entities[i], 1.0f,
                1.0f);

        std::map<Entity, int> visits;
        for (auto [entity, position, velocity] :
             registry.GetView<PositionComponent, VelocityComponent>())
        {
            ++visits[entity];
        }

        CheckVisitedOnce(visits, { entities[0], entities[1] });
    }

    TEST_CASE("Structured bindings are references to the stored components")
    {
        // `auto [...]` copies the tuple, but its elements are references
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        for (auto [entity, position, velocity] :
             registry.GetView<PositionComponent, VelocityComponent>())
        {
            static_assert(std::is_same_v<decltype(position),
                                         PositionComponent&>);
            position.x += velocity.vx;
        }

        for (const Entity entity : entities)
        {
            CHECK(registry.GetComponent<PositionComponent>(entity).x == 1.0f);
        }
    }

    TEST_CASE("Const components are bound as const references")
    {
        Registry registry;
        CreateEntities(registry);

        std::size_t visits = 0;
        for (auto [entity, position, velocity] :
             registry.GetView<const PositionComponent, VelocityComponent>())
        {
            static_assert(std::is_same_v<decltype(position),
                                         const PositionComponent&>);
            static_assert(std::is_same_v<decltype(velocity),
                                         VelocityComponent&>);
            velocity.vx = position.x + 3.0f;
            ++visits;
        }

        CHECK(visits == ENTITY_COUNT);
    }

    TEST_CASE("Empty views never enter the loop")
    {
        Registry registry;

        SUBCASE("a component type was never added")
        {
            CreateEntities(registry);
        }
        SUBCASE("the pools exist but are empty")
        {
            const std::vector<Entity> entities = CreateEntities(registry);
            for (const Entity entity : entities)
                registry.EmplaceComponent<HealthComponent>(entity, 100);
            for (const Entity entity : entities)
                registry.DestroyEntity(entity);
        }

        // HealthComponent is never added in the first subcase
        std::size_t visits = 0;
        for ([[maybe_unused]] auto components :
             registry.GetView<PositionComponent, HealthComponent>())
        {
            ++visits;
        }

        CHECK(visits == 0);
        CHECK(registry.GetView<PositionComponent, HealthComponent>().begin()
            == std::default_sentinel);
    }

    TEST_CASE("Destroying the current entity in the loop visits every entity once")
    {
        Registry registry;
        const std::vector<Entity> entities = CreateEntities(registry);

        std::map<Entity, int> visits;
        for (auto [entity, position, velocity] :
             registry.GetView<PositionComponent, VelocityComponent>())
        {
            ++visits[entity];

            if (libecs::core::GetEntityIndex(entity) % 4 == 0)
            {
                registry.DestroyEntity(entity);
            }
        }

        CheckVisitedOnce(visits, entities);
    }

    TEST_CASE("Works with <ranges> algorithms")
    {
        Registry registry;
        CreateEntities(registry, 2);

        auto view = registry.GetView<PositionComponent, VelocityComponent>();

        CHECK(std::ranges::distance(view) ==
            static_cast<std::ptrdiff_t>(ENTITY_COUNT / 2));

        const auto evenIndices = std::ranges::count_if(view,
            [](const auto& components) {
                return libecs::core::GetEntityIndex(
                    std::get<0>(components)) % 4 == 0;
            });
        CHECK(evenIndices == static_cast<std::ptrdiff_t>(ENTITY_COUNT / 4));
    }
}
