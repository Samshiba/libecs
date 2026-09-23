//
// Created by genin on 03/06/2026.
// Path: src/memory/PoolAllocator.cpp
//

#include <cassert>
#include <cstdint>
#include <new>
#include <stdexcept>

#include <libecs/memory/PoolAllocator.hpp>

#include "VirtualMemory.hpp"

namespace libecs::memory
{
    PoolAllocator::PoolAllocator(std::size_t chunkSize, std::size_t blockSize)
        : chunkSize_(chunkSize), blockSize_(blockSize)
    {
        if (chunkSize_ < sizeof(void*))
        {
            throw std::invalid_argument(
                "Chunk size must be at least sizeof(void*).");
        }

        if (blockSize < chunkSize_ || blockSize % chunkSize_ != 0)
        {
            throw std::invalid_argument(
                "Block size must be a multiple of chunk size and at least as large.");
        }

        start_ = detail::AllocatePages(blockSize_);
        if (!start_)
        {
            throw std::bad_alloc();
        }

        head_ = start_;

        char* currentPtr = static_cast<char*>(start_);
        std::size_t chunkCount = blockSize_ / chunkSize_;

        for (std::size_t i = 0; i < chunkCount - 1; ++i)
        {
            auto chunk = reinterpret_cast<Chunk*>(currentPtr);
            char* nextPtr = currentPtr + chunkSize_;
            chunk->next = reinterpret_cast<Chunk*>(nextPtr);
            currentPtr = nextPtr;
        }

        // Last chunk points to nullptr
        auto lastChunk = reinterpret_cast<Chunk*>(currentPtr);
        lastChunk->next = nullptr;
    }

    PoolAllocator::~PoolAllocator()
    {
        detail::FreePages(start_, blockSize_);
    }

    void* PoolAllocator::Allocate()
    {
        if (!head_)
        {
            return nullptr; // No more free space
        }

        auto chunk = reinterpret_cast<Chunk*>(head_);
        head_ = chunk->next;
        return chunk;
    }

    void PoolAllocator::Deallocate(void* ptr)
    {
        if (!ptr)
            return;

        // Only read by the assert: unused when NDEBUG is defined
        [[maybe_unused]] auto start = reinterpret_cast<std::uintptr_t>(start_);
        [[maybe_unused]] auto end = reinterpret_cast<std::uintptr_t>(ptr);

        assert(end >= start && end < start + blockSize_ && (end - start) %
            chunkSize_ == 0);

        auto* chunk = reinterpret_cast<Chunk*>(ptr);
        chunk->next = reinterpret_cast<Chunk*>(head_);
        head_ = chunk;
    }
}
