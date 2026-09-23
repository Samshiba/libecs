//
// Created by genin on 23/09/2026.
// Path: tests/core/view/View.test.cpp
//

#include <doctest.h>
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