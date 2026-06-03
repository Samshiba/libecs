//
// Created by genin on 03/06/2026.
// Path: include/libecs/memory/PoolAllocator.hpp
//

#pragma once

#include <cstddef>

namespace libecs::memory
{
    class PoolAllocator
    {
        struct Chunk
        {
            Chunk* next;
        };

    public:
        PoolAllocator(std::size_t chunkSize, std::size_t blockSize);
        ~PoolAllocator();

        void* Allocate();
        void Deallocate(void* ptr);

    private:
        void* start_ = nullptr;
        std::size_t chunkSize_;
        std::size_t blockSize_;
        void* head_;
    };
} // libecs::memory