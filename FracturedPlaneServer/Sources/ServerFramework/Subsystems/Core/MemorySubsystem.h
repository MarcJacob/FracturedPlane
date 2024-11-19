// HeapMemory.h
// Contains the Server Memory system that allows the managed allocation of heap memory provided by the platform.

#pragma once

#include <cstdint>

#define MEMORY_BLOCK_SIZE 32

// Server Management Object handling all memory allocations outside of other Server Management objects.
struct MemorySubsystem
{
    uint8_t* MemoryStart = nullptr;
    size_t MemorySize = 0;

    enum class MEMORY_BLOCK_STATE : uint8_t
    {
        FREE, // Available for allocation.
        ALLOCATED, // Allocated as first or only block in an allocation.
        ALLOCATED_EXTRA // Allocated as a contiguous allocation, "part of" another block.
    };
    
    struct MemoryBlock
    {
        // #TODO(Marc): It's out of extreme laziness that I'm adding the address in here.
        //void* Address = nullptr;
        MEMORY_BLOCK_STATE State = MEMORY_BLOCK_STATE::FREE;
    };
    MemoryBlock* MemoryBlocks = nullptr;
    size_t MemoryBlockCount = 0;

    bool Initialize(void* MemStart, size_t MemSize);
    void FreeServerHeap();

    // Allocates the requested amount of memory, zeroing it out first.
    void* Allocate(size_t Size);

    // Allocates the required amount of memory for the passed data type, times the requested count and runs construction.
    template <typename T>
    T* AllocateAndInit(size_t Count = 1)
    {
        T* Data = static_cast<T*>(Allocate(sizeof(T)));

        if (nullptr != Data)
        {
            for (size_t i = 0; i < Count; i++)
            {
                *(Data + i) = T();
            }
            return Data;
        }
        return nullptr;
    }

    // Allocates the required amount of memory for the passed data type, times the requested count and keeps it zeroed.
    template <typename T>
    T* AllocateZeroed(size_t Count = 1)
    {
        T* Data = static_cast<T*>(Allocate(sizeof(T) * Count));
        return Data;   
    }

    void Free(void* AllocatedAddress);  
};
