//
// Created by genin on 03/06/2026.
// Path: src/memory/LinearAllocator.cpp
//

#include <iostream>
#include <libecs/memory/LinearAllocator.hpp>

#ifdef LIBECS_PLATFORM_WINDOWS
#include <windows.h>
#include <memoryapi.h>
#else
#error "LinearAllocator is currently only implemented for Windows (mmap needed for POSIX)."
#endif

namespace libecs::memory
{
    LinearAllocator::LinearAllocator(std::size_t totalSize)
        : totalSize_(totalSize)
    {
        start_ = VirtualAlloc(nullptr, totalSize,
                              MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (!start_)
        {
            throw std::bad_alloc();
        }
    }

    LinearAllocator::~LinearAllocator()
    {
        if (start_)
        {
            VirtualFree(start_, 0, MEM_RELEASE);
        }
    }

    void* LinearAllocator::Allocate(std::size_t size, std::size_t alignment)
    {
        void* ptr = static_cast<char*>(start_) + offset_;

        std::size_t spaceLeft = totalSize_ - offset_;

        void* alignedPtr = std::align(alignment, size, ptr, spaceLeft);

        if (!alignedPtr)
        {
            throw std::bad_alloc();
        }

        offset_ = totalSize_ - spaceLeft + size;

        return alignedPtr;
    }

    void LinearAllocator::Clear()
    {
        offset_ = 0;
    }
} // memory