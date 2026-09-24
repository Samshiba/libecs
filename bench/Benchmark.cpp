//
// Created by genin on 24/09/2026.
// Path: bench/Benchmark.cpp
//
// Compares one "movement" update (position += velocity * dt) over 1M objects:
//   - a plain std::vector of structs (best case, theoretical ceiling)
//   - a classic OOP design (heap objects + virtual Update), in allocation
//     order and shuffled (objects created/destroyed over time)
//   - libecs views
//
// Build in Release: cmake -DLIBECS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
//

#include <algorithm>
#include <chrono>
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
    constexpr int MEASURED_RUNS = 30;
    constexpr float DT = 0.016f;

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

    // Runs `update` several times and returns the median duration in ms
    template <typename Func>
    double MeasureMedianMs(Func&& update)
    {
        for (int i = 0; i < WARMUP_RUNS; ++i)
        {
            update();
        }

        std::vector<double> timings;
        timings.reserve(MEASURED_RUNS);

        for (int i = 0; i < MEASURED_RUNS; ++i)
        {
            const auto start = std::chrono::steady_clock::now();
            update();
            const auto end = std::chrono::steady_clock::now();
            timings.push_back(
                std::chrono::duration<double, std::milli>(end - start).count());
        }

        std::sort(timings.begin(), timings.end());
        return timings[timings.size() / 2];
    }

    void PrintRow(const char* name, double ms, std::size_t processed,
                  float checksum)
    {
        const double nsPerEntity = ms * 1'000'000.0 / static_cast<double>(
            processed);
        // The checksum is printed so the compiler can't optimize the work away
        std::printf("| %-50s | %8.2f | %9.2f | %9zu | %g\n", name, ms,
                    nsPerEntity, processed, static_cast<double>(checksum));
    }

    // --- Plain vector of structs ------------------------------------------

    void BenchPlainVector()
    {
        struct Object
        {
            Position position;
            Velocity velocity;
        };

        std::vector<Object> objects(ENTITY_COUNT,
                                    Object{ { 0.0f, 0.0f }, { 1.0f, 2.0f } });

        const double ms = MeasureMedianMs([&] {
            for (Object& object : objects)
            {
                object.position.x += object.velocity.dx * DT;
                object.position.y += object.velocity.dy * DT;
            }
        });

        PrintRow("std::vector<struct> (baseline)", ms, ENTITY_COUNT,
                 objects.back().position.x);
    }

    // --- Classic OOP ------------------------------------------------------

    class GameObject
    {
    public:
        virtual ~GameObject() = default;
        virtual void Update(float dt) = 0;
    };

    class MovingObject final : public GameObject
    {
    public:
        void Update(float dt) override
        {
            position.x += velocity.dx * dt;
            position.y += velocity.dy * dt;
        }

        Position position{ 0.0f, 0.0f };
        Velocity velocity{ 1.0f, 2.0f };
    };

    void BenchOop(bool shuffled)
    {
        std::vector<std::unique_ptr<GameObject> > objects;
        objects.reserve(ENTITY_COUNT);

        for (std::size_t i = 0; i < ENTITY_COUNT; ++i)
        {
            objects.push_back(std::make_unique<MovingObject>());
        }

        if (shuffled)
        {
            std::mt19937 rng(42);
            std::shuffle(objects.begin(), objects.end(), rng);
        }

        const double ms = MeasureMedianMs([&] {
            for (auto& object : objects)
            {
                object->Update(DT);
            }
        });

        PrintRow(shuffled
                 ? "OOP: unique_ptr + virtual Update (shuffled)"
                 : "OOP: unique_ptr + virtual Update (in order)", ms,
                 ENTITY_COUNT,
                 static_cast<MovingObject&>(*objects.back()).position.x);
    }

    // --- libecs -----------------------------------------------------------

    // Every entity has a Position, one out of `velocityEvery` has a Velocity
    std::size_t Populate(libecs::core::registry::Registry& registry,
                         std::size_t velocityEvery)
    {
        std::size_t withVelocity = 0;

        for (std::size_t i = 0; i < ENTITY_COUNT; ++i)
        {
            const libecs::core::Entity entity = registry.CreateEntity();
            registry.EmplaceComponent<Position>(entity, 0.0f, 0.0f);

            if (i % velocityEvery == 0)
            {
                registry.EmplaceComponent<Velocity>(entity, 1.0f, 2.0f);
                ++withVelocity;
            }
        }

        return withVelocity;
    }

    void BenchView(const char* name, std::size_t velocityEvery)
    {
        libecs::core::registry::Registry registry;
        const std::size_t processed = Populate(registry, velocityEvery);

        auto view = registry.GetView<Position, Velocity>();
        float checksum = 0.0f;

        const double ms = MeasureMedianMs([&] {
            view.Each([](libecs::core::Entity, Position& position,
                         const Velocity& velocity) {
                position.x += velocity.dx * DT;
                position.y += velocity.dy * DT;
            });
        });

        view.Each([&](libecs::core::Entity, const Position& position,
                      const Velocity&) {
            checksum = position.x;
        });

        PrintRow(name, ms, processed, checksum);
    }

    // Same work, but the callback goes through std::function (type erasure):
    // shows what templating Each on the callable type buys
    void BenchViewStdFunction()
    {
        libecs::core::registry::Registry registry;
        const std::size_t processed = Populate(registry, 1);

        auto view = registry.GetView<Position, Velocity>();
        float checksum = 0.0f;

        std::function<void(libecs::core::Entity, Position&, Velocity&)> update
            = [](libecs::core::Entity, Position& position,
                 const Velocity& velocity) {
            position.x += velocity.dx * DT;
            position.y += velocity.dy * DT;
        };

        const double ms = MeasureMedianMs([&] {
            view.Each(update);
        });

        view.Each([&](libecs::core::Entity, const Position& position,
                      const Velocity&) {
            checksum = position.x;
        });

        PrintRow("libecs View<Position, Velocity> via std::function", ms,
                 processed, checksum);
    }
}

int main()
{
    std::printf("%zu entities, median of %d runs\n\n", ENTITY_COUNT,
                MEASURED_RUNS);
    std::printf("| %-50s | %8s | %9s | %9s | checksum\n", "Scenario",
                "ms", "ns/entity", "processed");
    std::printf("|%s|\n", std::string(90, '-').c_str());

    BenchPlainVector();
    BenchOop(false);
    BenchOop(true);
    BenchView("libecs View<Position, Velocity>", 1);
    BenchViewStdFunction();
    BenchView("libecs View<Position, Velocity>, 10% w/ Velocity", 10);

    return 0;
}
