// HeapMemory.cpp
// Implementation of Heap Memory management on the server.

#include "ServerFramework/Subsystems/Core/MemorySubsystem.h"
#include <iostream>


bool MemorySubsystem::Initialize(void* MemStart, size_t MemSize)
{
    // Build block bookkeeping data at the start of memory.
    MemoryBlockCount = MemSize / (MEMORY_BLOCK_SIZE + sizeof(MemoryBlock)); // Don't forget to take into account that memory bookkeeping data is also stored in memory !
    MemoryBlocks = static_cast<MemoryBlock*>(MemStart);
    for (size_t MemoryBlockIndex = 0; MemoryBlockIndex < MemoryBlockCount; MemoryBlockIndex++)
    {
        MemoryBlocks[MemoryBlockIndex] = MemoryBlock();
    }

    // Indicate where usable memory starts at (right after the bookkeeping data) and its actual size
    MemoryStart = static_cast<uint8_t*>(MemStart) + sizeof(MemoryBlock) * MemoryBlockCount;
    MemorySize = MemSize - sizeof(MemoryBlock) * MemoryBlockCount;
    
    return MemoryBlockCount > 0;
}

void MemorySubsystem::FreeServerHeap()
{
    memset(MemoryStart, 0, MemorySize);
    memset(MemoryBlocks, 0, MemoryBlockCount * sizeof(MemoryBlock));
    MemoryBlockCount = 0;
}

void* MemorySubsystem::Allocate(size_t Size)
{
    size_t RequiredBlockCount = Size / MEMORY_BLOCK_SIZE + (Size == MEMORY_BLOCK_SIZE ? 0 : 1);

    size_t BlockIndex;
    // #TODO: Add extra layer of bookkeeping to more easily find "blocks of blocks" of a certain contiguous size.
    for(BlockIndex = 0; BlockIndex < MemoryBlockCount; BlockIndex++)
    {
        size_t LookAheadIndex = 0;
        while(BlockIndex + LookAheadIndex < MemoryBlockCount && MemoryBlocks[BlockIndex + LookAheadIndex].State == MEMORY_BLOCK_STATE::FREE
            && LookAheadIndex < RequiredBlockCount)
        {
            LookAheadIndex++;
        }

        if (LookAheadIndex == RequiredBlockCount)
        {
            break;
        }
    }

    if (BlockIndex < MemoryBlockCount)
    {
        uint8_t* AllocatableMemoryStart = MemoryStart + BlockIndex * MEMORY_BLOCK_SIZE;
        
        // Allocate First Block
        MemoryBlocks[BlockIndex].State = MEMORY_BLOCK_STATE::ALLOCATED;

        // Allocate Extra blocks if needed
        for(size_t ExtraBlockIndex = BlockIndex + 1; ExtraBlockIndex < BlockIndex + RequiredBlockCount; ExtraBlockIndex++)
        {
            MemoryBlocks[ExtraBlockIndex].State = MEMORY_BLOCK_STATE::ALLOCATED_EXTRA;
        }

        // Zero out allocated memory
        memset(AllocatableMemoryStart, 0, MEMORY_BLOCK_SIZE * RequiredBlockCount);
        
        return AllocatableMemoryStart;
    }

    std::cerr << "Memory allocation failed ! Requested bytes = " << Size << "\n";
    return nullptr;
}

void MemorySubsystem::Free(void* AllocatedAddress)
{
    intptr_t AddressOffset = reinterpret_cast<intptr_t>(AllocatedAddress) - reinterpret_cast<intptr_t>(MemoryStart);
    if (AddressOffset < 0 || static_cast<size_t>(AddressOffset) > MemorySize && AddressOffset % MEMORY_BLOCK_SIZE != 0)
    {
        std::cerr << "Failed to free memory: Passed Address is invalid.\n";
        return;
    }

    // Get Block Index from the address offset. The previous checks should guarantee that it lands on a valid value.
    size_t BlockIndex = AddressOffset / MEMORY_BLOCK_SIZE;

    // Free start block then start freeing subsequent "Extra" blocks that must have been part of the same allocation.
    MemoryBlocks[BlockIndex].State = MEMORY_BLOCK_STATE::FREE;
    for(size_t ExtraBlockIndex = BlockIndex + 1; ExtraBlockIndex < MemoryBlockCount; ExtraBlockIndex++)
    {
        if (MemoryBlocks[ExtraBlockIndex].State == MEMORY_BLOCK_STATE::ALLOCATED_EXTRA)
        {
            MemoryBlocks[ExtraBlockIndex].State = MEMORY_BLOCK_STATE::FREE;
        }
    }
}
