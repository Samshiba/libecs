# libecs

[![CI](https://github.com/Samshiba/libecs/actions/workflows/ci.yaml/badge.svg)](https://github.com/Samshiba/libecs/actions/workflows/ci.yaml)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

A small Entity Component System written in C++20, with no dependencies.

Entities are plain integer handles. Each component type is stored in its own sparse set, which keeps the components contiguous in memory. Views let you iterate over every entity that has a given set of components.

I wrote it to learn how an ECS works from the inside, and it is meant to become the core of my game engine.

## Highlights

- **Safe entity handles**: each handle carries a version number. A handle to a destroyed entity is detected, and never silently points to the new entity reusing its slot.
- **O(1) add, remove and lookup** of components, which are stored contiguously in memory.
- **Multi-component views** that iterate the smallest pool first.
- **Only reads the components a system needs**: on 1 million objects with 6 components each, a view is about 3× faster than a plain array of structs and 4 to 10× faster than a classic object-oriented loop on my laptop (see [Benchmarks](#benchmarks)).
- **Tested on Windows and Linux** with MSVC, GCC and Clang, on every push.

## Example

```cpp
#include <libecs/core/registry/Registry.hpp>

struct Position { float x, y; };
struct Velocity { float dx, dy; };

int main()
{
    libecs::core::registry::Registry registry;

    const libecs::core::Entity player = registry.CreateEntity();
    registry.EmplaceComponent<Position>(player, 0.0f, 0.0f);
    registry.EmplaceComponent<Velocity>(player, 1.0f, 0.5f);

    const libecs::core::Entity tree = registry.CreateEntity();
    registry.EmplaceComponent<Position>(tree, 10.0f, 3.0f); // no Velocity: won't move

    // A "system" is just code iterating a view
    const float dt = 0.016f;
    registry.GetView<Position, Velocity>().Each(
        [dt](libecs::core::Entity, Position& position, const Velocity& velocity) {
            position.x += velocity.dx * dt;
            position.y += velocity.dy * dt;
        });

    // Optional components
    if (const Velocity* velocity = registry.TryGetComponent<Velocity>(tree))
    {
        // not reached: the tree has no Velocity
    }

    registry.DestroyEntity(tree); // also removes its components
    // registry.IsEntityValid(tree) == false: the old handle is now stale
}
```

## Benchmarks

Each scenario times one movement update (`position += velocity * dt`) over **1,000,000 objects**, compared across three designs:

- a plain `std::vector` of structs;
- classic OOP: objects allocated one by one on the heap and updated through a virtual `Update`, either in allocation order or shuffled, which simulates objects created and destroyed during a game;
- a libecs view.

Every case is measured twice: with objects holding only the 2 components the update needs, then with more realistic objects holding 6 (`Position`, `Velocity`, `Rotation`, `Scale`, `Health`, `Sprite`: 56 bytes, of which the update uses 16). To limit noise, the scenarios are interleaved over 5 rounds of 10 runs each.

Median times, Release build, AMD Ryzen 7 5800H laptop (16 MB of L3 cache):

| Scenario                                        |    MSVC 19.42 |       GCC 13.3 |     Clang 18.1 |
|-------------------------------------------------|--------------:|---------------:|---------------:|
| **2 components per object**                     |               |                |                |
| `std::vector<struct>`                           |        1.8 ms |         1.7 ms |         1.7 ms |
| OOP, allocation order                           |        6.6 ms |         4.2 ms |         4.9 ms |
| OOP, shuffled                                   |       20.0 ms |        18.9 ms |        18.9 ms |
| **libecs** `View<Position, Velocity>`           |    **2.3 ms** |     **2.1 ms** |     **2.1 ms** |
| libecs view, `std::function` callback           |        3.3 ms |         3.2 ms |         3.2 ms |
| libecs view, 10% of entities have a `Velocity`  |        1.3 ms |         1.3 ms |         1.4 ms |
| **6 components per object**                     |               |                |                |
| `std::vector<struct>`                           |        6.7 ms |         6.5 ms |         6.5 ms |
| OOP, allocation order                           |       14.7 ms |         8.4 ms |        10.0 ms |
| OOP, shuffled                                   |       21.2 ms |        20.0 ms |        19.7 ms |
| **libecs** `View<Position, Velocity>`           |    **2.3 ms** |     **1.9 ms** |     **2.3 ms** |

How to read these numbers:

- **The view only pays for what it reads.** Its time is the same with 2 or 6 components per object, because it only walks the `Position` and `Velocity` arrays. An array of structs, or an OOP object, loads the whole 56-byte object to use 16 bytes of it. With 6 components, the view is about 3× faster than the array of structs, 4 to 6× faster than OOP in allocation order and 9 to 10× faster than shuffled OOP.
- **With only 2 components, a plain array of structs stays 1.2 to 1.4× faster.** That is its best case: each object holds exactly what the update needs and nothing else. The view pays for a lookup in the `Velocity` sparse set for every entity, which is the price of being able to add and remove components at runtime.
- **`Each` takes the callback as a template parameter**, so the compiler can inline it. Passing the same callback through `std::function` blocks inlining and makes the loop 40 to 50% slower.
- **Starting from the smallest pool pays off.** When only 100k of the 1M entities have a `Velocity`, the view walks the `Velocity` pool and takes 1.3 ms instead of 2.2 ms. Each matching entity costs more, though, because its `Position` is no longer read sequentially.

**These results depend on the hardware.** On my laptop, 1M objects don't fit in the CPU cache, so the update is limited by memory bandwidth, and reading less data is what matters most. The GitHub Actions runners have larger caches: there, the arrays of structs stay in cache, and the plain `std::vector<struct>` gets 3 to 5× faster than the view with 2 components. With 6 components, the view is still faster with GCC and Clang (1.4 to 2×), but slower with MSVC (1.5×). The OOP loops stay slower than the view everywhere: 1.2 to 1.8× in allocation order with 2 components, and 2.5 to 9× in every other case. Each CI run publishes its results in the job summary.

To run the benchmark yourself:

```bash
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DLIBECS_BUILD_BENCHMARKS=ON
cmake --build build-bench
./build-bench/bin/libecs_bench
```

## How it works

### Entity handles

```
 31                                    8 7          0
┌───────────────────────────────────────┬────────────┐
│             index (24 bits)           │ version (8)│
└───────────────────────────────────────┴────────────┘
```

The registry keeps one slot per index. When an entity is destroyed, its slot stores the index of the next free slot. The free slots therefore form a linked list inside the array itself, with no extra allocation. `CreateEntity` reuses the first free slot and increments its version. A handle kept from before the destruction has the old version, so `IsEntityValid` rejects it.

### Sparse sets

Each component type is stored in its own `SparseSet<T>`:

```
entity index:        3      7      12
                     │      │      │
sparse (paged):   [3]→0  [7]→2  [12]→1       entity index → position in the dense arrays
                     │      │      │
dense entities:   [  e3  |  e12  |  e7  ]    contiguous
dense components: [  C3  |  C12  |  C7  ]    contiguous, same order
```

- **Lookup**: `sparse[index]` gives the position in the dense arrays. The entity stored at that position is then compared with the full handle, version included.
- **Removal**: the removed element is swapped with the last one, then popped. The dense arrays never contain holes, so iterating them is just a linear scan.
- **Pages**: the sparse array is split into pages of 4096 entries, allocated only when needed. A single entity with index 1,000,000 costs one page, not an array of a million entries.

### Component pools

The registry owns one pool per component type, created the first time a component of that type is added. The pools are stored as `std::unique_ptr<IPool>`, indexed by a numeric id generated once per type.

When the component type is known at compile time, which is the case in `EmplaceComponent<T>`, `GetView<Ts...>` and the rest of the templated API, the pool is cast back to `SparseSet<T>` directly, with no runtime cost. Only `DestroyEntity`, which has to visit every pool without knowing their types, goes through the virtual `IPool` interface.

### Views

A `View<Ts...>` only holds one pointer per pool, so it is cheap to create every frame. `Each` picks the pool with the fewest components and walks it by position in its dense arrays: its components are read directly, without any lookup. For every other pool, a single sparse set lookup either returns the entity's component or tells that the entity doesn't have it, in which case the entity is skipped. The cost is therefore proportional to the smallest pool, not to the total number of entities.

The smallest pool is only known at runtime, but reading it without lookups needs its type at compile time. `Each` bridges the two: it generates one version of the loop per possible pool (with `std::index_sequence`), and calls the one matching the smallest pool.

The loop walks the pool backwards. When the callback destroys the current entity, swap-and-pop moves the last element into its slot, and that element has already been visited. The entities still to visit never move, so none is skipped or visited twice.

## Getting started

libecs needs **CMake 3.28** or newer and a **C++20** compiler. It is tested with MSVC 19.42, GCC 13 and Clang 18.

### Add it to your project

With **FetchContent** (recommended):

```cmake
include(FetchContent)
FetchContent_Declare(libecs
    GIT_REPOSITORY https://github.com/Samshiba/libecs.git
    GIT_TAG        v0.1.0)
FetchContent_MakeAvailable(libecs)

target_link_libraries(my_engine PRIVATE libecs::libecs)
```

With a **git submodule**:

```cmake
add_subdirectory(external/libecs)
target_link_libraries(my_engine PRIVATE libecs::libecs)
```

In both cases, libecs detects that it is not the main project. It then skips its tests, does not generate install rules, and does not turn warnings into errors, so it never changes how your own project builds.

As an **installed package**:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix <install-dir>
```

```cmake
find_package(libecs 0.1 REQUIRED)   # configure with -DCMAKE_PREFIX_PATH=<install-dir>
target_link_libraries(my_engine PRIVATE libecs::libecs)
```

### Build and run the tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

| Option                      | Default                  | Description                                                                    |
|-----------------------------|--------------------------|--------------------------------------------------------------------------------|
| `LIBECS_BUILD_TESTS`        | `ON` as the main project | Build the unit tests ([doctest](https://github.com/doctest/doctest), vendored) |
| `LIBECS_BUILD_BENCHMARKS`   | `OFF`                    | Build `libecs_bench`                                                           |
| `LIBECS_INSTALL`            | `ON` as the main project | Generate the install and `find_package` rules                                  |
| `LIBECS_WARNINGS_AS_ERRORS` | `ON` as the main project | Add `/WX` (MSVC) or `-Werror` (GCC, Clang)                                     |

Besides `Debug` and `Release`, a `Profile` configuration builds with optimizations and debug symbols.

## Limitations

- **Not thread-safe.** A registry must be used from one thread at a time.
- **Inside `Each`, only modify the current entity.** Destroying it or removing its components is safe. Destroying another entity can make the loop visit an entity twice, and adding a component of a viewed type can reallocate the arrays being iterated. Collect those changes and apply them after the loop.
- **The iteration order of a view is unspecified.** It depends on the pool being walked and changes as components are removed.
- **Create views right before using them.** A view created before the first component of one of its types was added stays empty.
- **Versions are 8 bits**, so they wrap around after 256 reuses of the same slot, and a very old handle could then look valid again.
- **At most 16,777,215 entities** can exist at the same time (24-bit index).
- **Component type ids depend on the order in which the types are first used**, so they can differ from one run to the next. Don't save them to disk.

## Roadmap

- [x] Look each component up only once per entity in `Each`, and skip the lookups in the pool being iterated
- [x] Iterate backwards in `Each`, so that removing the current entity's components becomes safe
- [ ] Range-based `for` over views: `for (auto [entity, position, velocity] : view)`
- [ ] Read-only views: `View<const Position>`

## License

[MIT](LICENSE) © Jules Genin
