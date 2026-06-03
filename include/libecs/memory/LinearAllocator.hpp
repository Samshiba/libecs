//
// Created by genin on 03/06/2026.
// Path: include/libecs/memory/LinearAllocator.hpp
//

#pragma once

#include <cstddef>

namespace libecs::memory
{
    class LinearAllocator
    {
    public:
        LinearAllocator(std::size_t totalSize);
        ~LinearAllocator();

        void* Allocate(std::size_t size, std::size_t alignment);
        void Clear();

        void* GetBaseAddress() const
        {
            return start_;
        }

    private:
        void* start_ = nullptr;
        std::size_t offset_ = 0;
        std::size_t totalSize_ = 0;
    };
} // libecs::memory