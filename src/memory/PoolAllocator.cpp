//
// Created by genin on 03/06/2026.
// Path: src/memory/PoolAllocator.cpp
//

#include <iostream>
#include <libecs/memory/PoolAllocator.hpp>

#ifdef LIBECS_PLATFORM_WINDOWS
#include <windows.h>
#include <memoryapi.h>
#else
#error "PoolAllocator is currently only implemented for Windows (mmap needed for POSIX)."
#endif

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

        start_ = VirtualAlloc(nullptr, blockSize_,
                              MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
        if (!start_)
        {
            std::cerr << "Failed to reserve memory for PoolAllocator.\n";
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
        if (start_)
        {
            VirtualFree(start_, 0, MEM_RELEASE);
        }
    }

    void* PoolAllocator::Allocate()
    {
        if (!head_)
        {
            std::cerr << "PoolAllocator out of memory.\n";
            return nullptr;
        }

        auto chunk = reinterpret_cast<Chunk*>(head_);
        head_ = chunk->next;
        return chunk;
    }

    void PoolAllocator::Deallocate(void* ptr)
    {
        if (!ptr)
            return;

        auto start = reinterpret_cast<std::uintptr_t>(start_);
        auto end = reinterpret_cast<std::uintptr_t>(ptr);

        if (end < start || end >= start + blockSize_ || (end - start) %
            chunkSize_ != 0)
        {
            std::cerr <<
                "Pointer out of bounds for PoolAllocator deallocation.\n";
            return;
        }

        auto* chunk = reinterpret_cast<Chunk*>(ptr);
        chunk->next = reinterpret_cast<Chunk*>(head_);
        head_ = chunk;
    }
}
