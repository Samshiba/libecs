//
// Created by genin on 23/09/2026.
// Path: src/memory/VirtualMemory.cpp
//

#include "VirtualMemory.hpp"

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#else
#   include <sys/mman.h>
#endif

namespace libecs::memory::detail
{
    void* AllocatePages(std::size_t size)
    {
#if defined(_WIN32)
        return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT,
                            PAGE_READWRITE);
#else
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return ptr == MAP_FAILED ? nullptr : ptr;
#endif
    }

    void FreePages(void* ptr, [[maybe_unused]] std::size_t size)
    {
        if (!ptr)
        {
            return;
        }

#if defined(_WIN32)
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
        munmap(ptr, size);
#endif
    }
}
