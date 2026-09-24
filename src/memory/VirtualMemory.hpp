//
// Created by genin on 23/09/2026.
// Path: src/memory/VirtualMemory.hpp
//

#pragma once

#include <cstddef>

namespace libecs::memory::detail
{
    // Reserves and commits `size` bytes of zeroed, read-write memory.
    // Returns nullptr on failure.
    void* AllocatePages(std::size_t size);

    // Releases memory obtained from AllocatePages. `size` must be the size
    // passed to AllocatePages.
    void FreePages(void* ptr, std::size_t size);
}
