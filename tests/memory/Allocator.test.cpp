//
// Created by genin on 03/06/2026.
// Path: tests/memory/Allocator.test.cpp
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <libecs/memory/LinearAllocator.hpp>
#include  <libecs/memory/PoolAllocator.hpp>

TEST_CASE("LinearAllocator basic functionality")
{
    const size_t allocatorSize = 1024;
    libecs::memory::LinearAllocator allocator(allocatorSize);

    // Test allocation
    void* ptr1 = allocator.Allocate(256, 16);
    REQUIRE(ptr1 != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr1) % 16 == 0);

    void* ptr2 = allocator.Allocate(128, 32);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr2) % 32 == 0);

    // Test that allocations are within bounds
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(allocator.
        GetBaseAddress());
    uintptr_t endAddress = baseAddress + allocatorSize;
    REQUIRE(reinterpret_cast<uintptr_t>(ptr1) >= baseAddress);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr1) < endAddress);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr2) >= baseAddress);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr2) < endAddress);

    // Test that we can allocate until we run out of space
    void* ptr3 = allocator.Allocate(512, 64);
    REQUIRE(ptr3 != nullptr);

    void* ptr4 = allocator.Allocate(256, 16);
    REQUIRE(ptr4 == nullptr);

    // Test reset functionality
    allocator.Clear();
    void* ptr5 = allocator.Allocate(512, 64);
    REQUIRE(ptr5 != nullptr);
}

TEST_CASE("PoolAllocator basic functionality")
{
    const size_t objectSize = 64;
    const size_t poolSize = 10;
    libecs::memory::PoolAllocator allocator(objectSize, objectSize * poolSize);

    // Test allocation
    void* ptrs[poolSize];
    for (size_t i = 0; i < poolSize; ++i)
    {
        ptrs[i] = allocator.Allocate();
        REQUIRE(ptrs[i] != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptrs[i]) % objectSize == 0);
    }

    // Test that we can allocate until we run out of space
    void* extraPtr = allocator.Allocate();
    REQUIRE(extraPtr == nullptr);

    // Test deallocation and reuse
    allocator.Deallocate(ptrs[0]);
    void* newPtr = allocator.Allocate();
    REQUIRE(newPtr == ptrs[0]);
}