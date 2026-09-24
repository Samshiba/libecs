//
// Created by genin on 03/06/2026.
// Path: src/memory/LinearAllocator.cpp
//

#include <memory>
#include <new>

#include <libecs/memory/LinearAllocator.hpp>

#include "VirtualMemory.hpp"

namespace libecs::memory
{
    LinearAllocator::LinearAllocator(std::size_t totalSize)
        : totalSize_(totalSize)
    {
        start_ = detail::AllocatePages(totalSize_);

        if (!start_)
        {
            throw std::bad_alloc();
        }
    }

    LinearAllocator::~LinearAllocator()
    {
        detail::FreePages(start_, totalSize_);
    }

    void* LinearAllocator::Allocate(std::size_t size, std::size_t alignment)
    {
        void* ptr = static_cast<char*>(start_) + offset_;

        std::size_t spaceLeft = totalSize_ - offset_;

        void* alignedPtr = std::align(alignment, size, ptr, spaceLeft);

        if (!alignedPtr)
        {
            return nullptr; // No more free space
        }

        offset_ = totalSize_ - spaceLeft + size;

        return alignedPtr;
    }

    void LinearAllocator::Clear()
    {
        offset_ = 0;
    }
} // memory