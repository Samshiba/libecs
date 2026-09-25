//
// Created by genin on 24/09/2026.
// Path: bench/Benchmark.cpp
//
// Times one "movement" update (position += velocity * dt) over 1M objects:
//   - a plain std::vector of structs
//   - a classic OOP design (heap objects + virtual Update), in allocation
//     order and shuffled (objects created/destroyed over time)
//   - libecs views, through Each and through a range-based for
//
// Each case runs twice: with objects holding only the 2 components the update
// needs, then with "real" objects holding 6 components (the update still only
// touches 2 of them).
//
// To limit noise, the scenarios are interleaved: every round runs all of them
// in a shuffled order, so CPU frequency changes or background processes weigh
// on every scenario alike. Both the median and the minimum are reported (noise
// only ever slows a run down, so the minimum is the closest to the real cost).
//
// Build in Release: cmake -DLIBECS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
//

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <libecs/core/registry/Registry.hpp>

namespace
{
    constexpr std::size_t ENTITY_COUNT = 1'000'000;
    constexpr int WARMUP_RUNS = 3;
    constexpr int ROUNDS = 5;
    constexpr int RUNS_PER_ROUND = 10;
    constexpr float DT = 0.016f;

    using libecs::core::Entity;
    using libecs::core::registry::Registry;

    // --- Components -------------------------------------------------------

    struct Position
    {
        float x;
        float y;
    };

    struct Velocity
    {
        float dx;
        float dy;
    };

    // Components the movement update doesn't need, but a real game object has
    struct Rotation
    {
        float angle;
    };

    struct Scale
    {
        float x;
        float y;
    };

    struct Health
    {
        float current;
        float max;
    };

    struct Sprite
    {
        std::uint32_t textureId;
        float u0;
        float v0;
        float u1;
        float v1;
    };

    // --- Objects for the non-ECS scenarios --------------------------------

    struct SmallObject
    {
        Position position{ 0.0f, 0.0f };
        Velocity velocity{ 1.0f, 2.0f };
    };

    // 56 bytes, of which the movement update uses 16
    struct FatObject
    {
        Position position{ 0.0f, 0.0f };
        Velocity velocity{ 1.0f, 2.0f };
        Rotation rotation{ 0.0f };
        Scale scale{ 1.0f, 1.0f };
        Health health{ 100.0f, 100.0f };
        Sprite sprite{ 0, 0.0f, 0.0f, 1.0f, 1.0f };
    };

    class GameObject
    {
    public:
        virtual ~GameObject() = default;
        virtual void Update(float dt) = 0;
        virtual float Checksum() const = 0;
    };

    template <typename Data>
    class MovingObject final : public GameObject
    {
    public:
        void Update(float dt) override
        {
            data_.position.x += data_.velocity.dx * dt;
            data_.position.y += data_.velocity.dy * dt;
        }

        float Checksum() const override
        {
            return data_.position.x;
        }

    private:
        Data data_;
    };

    // --- Scenario ---------------------------------------------------------

    struct Scenario
    {
        std::string name;
        std::size_t processed;
        std::function<void()> update;
        // position.x of one updated object: equal across scenarios when they
        // all did the same work, and keeps the compiler from removing it
        std::function<float()> checksum;
        std::vector<double> timings{};
    };

    template <typename Object>
    Scenario MakePlainVector(std::string name)
    {
        auto objects = std::make_shared<std::vector<Object> >(ENTITY_COUNT);

        return {
            std::move(name), ENTITY_COUNT,
            [objects] {
                for (Object& object : *objects)
                {
                    object.position.x += object.velocity.dx * DT;
                    object.position.y += object.velocity.dy * DT;
                }
            },
            [objects] { return objects->front().position.x; }
        };
    }

    template <typename Data>
    Scenario MakeOop(std::string name, bool shuffled)
    {
        using Objects = std::vector<std::unique_ptr<GameObject> >;
        auto objects = std::make_shared<Objects>();
        objects->reserve(ENTITY_COUNT);

        for (std::size_t i = 0; i < ENTITY_COUNT; ++i)
        {
            objects->push_back(std::make_unique<MovingObject<Data> >());
        }

        if (shuffled)
        {
            std::mt19937 rng(42);
            std::shuffle(objects->begin(), objects->end(), rng);
        }

        return {
            std::move(name), ENTITY_COUNT,
            [objects] {
                for (auto& object : *objects)
                {
                    object->Update(DT);
                }
            },
            [objects] { return objects->front()->Checksum(); }
        };
    }

    struct EcsWorld
    {
        Registry registry;
        Entity first = libecs::core::NULL_ENTITY;
        std::size_t moving = 0;
    };

    // Every entity has a Position, one out of `velocityEvery` a Velocity.
    // With `fat`, every entity also gets the 4 components the update ignores.
    std::shared_ptr<EcsWorld> MakeWorld(std::size_t velocityEvery, bool fat)
    {
        auto world = std::make_shared<EcsWorld>();

        for (std::size_t i = 0; i < ENTITY_COUNT; ++i)
        {
            const Entity entity = world->registry.CreateEntity();
            world->registry.EmplaceComponent<Position>(entity, 0.0f, 0.0f);

            if (i % velocityEvery == 0)
            {
                world->registry.EmplaceComponent<Velocity>(entity, 1.0f, 2.0f);
                ++world->moving;
            }

            if (fat)
            {
                world->registry.EmplaceComponent<Rotation>(entity, 0.0f);
                world->registry.EmplaceComponent<Scale>(entity, 1.0f, 1.0f);
                world->registry.EmplaceComponent<Health>(entity, 100.0f,
                                                         100.0f);
                world->registry.EmplaceComponent<Sprite>(
                    entity, std::uint32_t{ 0 }, 0.0f, 0.0f, 1.0f, 1.0f);
            }

            if (i == 0)
            {
                world->first = entity;
            }
        }

        return world;
    }

    void MoveOne(Entity, Position& position, const Velocity& velocity)
    {
        position.x += velocity.dx * DT;
        position.y += velocity.dy * DT;
    }

    Scenario MakeView(std::string name, std::size_t velocityEvery, bool fat)
    {
        auto world = MakeWorld(velocityEvery, fat);

        return {
            std::move(name), world->moving,
            [world] {
                world->registry.GetView<Position, Velocity>().Each(
                    [](Entity entity, Position& position,
                       const Velocity& velocity) {
                        MoveOne(entity, position, velocity);
                    });
            },
            [world] {
                return world->registry.GetComponent<Position>(world->first).x;
            }
        };
    }

    // Same work with a range-based for: more convenient, but the iterator
    // can't skip the lookups in the iterated pool like Each does
    Scenario MakeViewRangeFor(std::string name, bool fat)
    {
        auto world = MakeWorld(1, fat);

        return {
            std::move(name), world->moving,
            [world] {
                for (auto [entity, position, velocity] :
                     world->registry.GetView<Position, Velocity>())
                {
                    MoveOne(entity, position, velocity);
                }
            },
            [world] {
                return world->registry.GetComponent<Position>(world->first).x;
            }
        };
    }

    // Same work, but the callback goes through std::function (type erasure):
    // shows what taking the callable as a template parameter buys
    Scenario MakeViewStdFunction(std::string name)
    {
        auto world = MakeWorld(1, false);
        auto callback = std::make_shared<std::function<void(
            Entity, Position&, Velocity&)> >(
            [](Entity entity, Position& position, const Velocity& velocity) {
                MoveOne(entity, position, velocity);
            });

        return {
            std::move(name), world->moving,
            [world, callback] {
                world->registry.GetView<Position, Velocity>().Each(*callback);
            },
            [world] {
                return world->registry.GetComponent<Position>(world->first).x;
            }
        };
    }

    // --- Measurement ------------------------------------------------------

    void Measure(std::vector<Scenario*>& scenarios)
    {
        for (Scenario* scenario : scenarios)
        {
            for (int run = 0; run < WARMUP_RUNS; ++run)
            {
                scenario->update();
            }
        }

        std::mt19937 rng(1234);

        for (int round = 0; round < ROUNDS; ++round)
        {
            std::shuffle(scenarios.begin(), scenarios.end(), rng);

            for (Scenario* scenario : scenarios)
            {
                for (int run = 0; run < RUNS_PER_ROUND; ++run)
                {
                    const auto start = std::chrono::steady_clock::now();
                    scenario->update();
                    const auto end = std::chrono::steady_clock::now();

                    scenario->timings.push_back(
                        std::chrono::duration<double, std::milli>(end - start).
                        count());
                }
            }
        }
    }

    const char* CompilerName()
    {
#if defined(__clang__)
        return "Clang " __clang_version__;
#elif defined(__GNUC__)
        return "GCC " __VERSION__;
#elif defined(_MSC_VER)
        static const std::string name = "MSVC " + std::to_string(_MSC_VER);
        return name.c_str();
#else
        return "unknown compiler";
#endif
    }

    // Prints a Markdown table: readable in a terminal, rendered by GitHub
    // (names use backticks, or GitHub would read <struct> as an HTML tag)
    void PrintTable(const char* title, const std::vector<Scenario>& scenarios)
    {
        std::printf("\n### %s\n\n", title);
        std::printf("| %-48s | %8s | %8s | %9s | %9s | %8s |\n", "Scenario",
                    "median", "min", "ns/entity", "processed", "checksum");
        std::printf("|%s|%s:|%s:|%s:|%s:|%s:|\n", std::string(50, '-').c_str(),
                    std::string(9, '-').c_str(), std::string(9, '-').c_str(),
                    std::string(10, '-').c_str(), std::string(10, '-').c_str(),
                    std::string(9, '-').c_str());

        for (const Scenario& scenario : scenarios)
        {
            std::vector<double> timings = scenario.timings;
            std::sort(timings.begin(), timings.end());

            const double median = timings[timings.size() / 2];
            const double nsPerEntity = median * 1'000'000.0 /
                static_cast<double>(scenario.processed);

            std::printf("| %-48s | %5.2f ms | %5.2f ms | %9.2f | %9zu | %8g |\n",
                        scenario.name.c_str(), median, timings.front(),
                        nsPerEntity, scenario.processed,
                        static_cast<double>(scenario.checksum()));
        }
    }
}

int main()
{
    std::printf("%zu entities, %s, %d rounds x %d runs per scenario "
                "(interleaved)\n", ENTITY_COUNT, CompilerName(), ROUNDS,
                RUNS_PER_ROUND);

    std::vector<Scenario> small;
    small.push_back(MakePlainVector<SmallObject>("`std::vector<struct>`"));
    small.push_back(MakeOop<SmallObject>("OOP, allocation order", false));
    small.push_back(MakeOop<SmallObject>("OOP, shuffled", true));
    small.push_back(MakeView("libecs `View<Position, Velocity>`", 1, false));
    small.push_back(MakeViewRangeFor("libecs view, range-based `for`", false));
    small.push_back(MakeViewStdFunction(
        "libecs view, `std::function` callback"));
    small.push_back(MakeView("libecs view, 10% of entities have a `Velocity`",
                             10, false));

    std::vector<Scenario> fat;
    fat.push_back(MakePlainVector<FatObject>("`std::vector<struct>`"));
    fat.push_back(MakeOop<FatObject>("OOP, allocation order", false));
    fat.push_back(MakeOop<FatObject>("OOP, shuffled", true));
    fat.push_back(MakeView("libecs `View<Position, Velocity>`", 1, true));
    fat.push_back(MakeViewRangeFor("libecs view, range-based `for`", true));

    // All scenarios are interleaved together
    std::vector<Scenario*> all;
    for (Scenario& scenario : small)
        all.push_back(&scenario);
    for (Scenario& scenario : fat)
        all.push_back(&scenario);

    Measure(all);

    PrintTable("Objects with 2 components (Position, Velocity)", small);
    PrintTable("Objects with 6 components (the update still uses 2)", fat);

    return 0;
}
