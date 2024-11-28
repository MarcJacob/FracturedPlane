#include "ServerFramework/Subsystems/Core/WorldSubsystem.h"
#include "ServerFramework/Subsystems/Core/MemorySubsystem.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

#include "ServerFramework/Subsystems/Net/ClientsSubsystem.h"

bool WorldSubsystem::GenerateWorldLandscape(MemorySubsystem& Memory, WorldSize_t Size)
{
    using namespace FPCore::World;
    
    size_t TotalTileCount = static_cast<size_t>(Size) * Size;
    
    WorldLandscape.Size = Size;
    WorldLandscape.TileTypes = Memory.AllocateZeroed<FPCore::World::TileLandscapeType>(TotalTileCount);
    WorldLandscape.TileAltitudes = Memory.AllocateZeroed<FPCore::World::TileAltitude_t>(TotalTileCount);

    if (nullptr == WorldLandscape.TileTypes || nullptr == WorldLandscape.TileAltitudes)
    {
        return false;
    }
    
    // Randomize data
    srand(time(NULL));

    for(size_t TileIndex = 0; TileIndex < TotalTileCount; TileIndex++)
    {
        WorldLandscape.TileTypes[TileIndex] = static_cast<TileLandscapeType>(rand() % static_cast<int>(TileLandscapeType::TYPE_COUNT));
        WorldLandscape.TileAltitudes[TileIndex] = (rand() % 8000) - 4000; 
    }

    bWorldGenerated = true;
    return true;
}

bool WorldSubsystem::InitializeEntityData(MemorySubsystem& Memory, size_t CharacterCount)
{
    // Initialize Character Data Buffers
    {
        MaxCharacterCount = CharacterCount;
        CharacterActiveFlags = Memory.AllocateZeroed<bool>(CharacterCount);
        CharacterCoordinates = Memory.AllocateZeroed<FPCore::World::Coordinates>(CharacterCount);
        CharacterNames = Memory.AllocateZeroed<FPCore::World::CharacterName>(CharacterCount);

        if (CharacterActiveFlags == nullptr
            || CharacterCoordinates == nullptr
            || CharacterNames == nullptr)
        {
            std::cerr << "Error: Failed to allocate Character Data Buffers !\n";
            return false;
        }
    }

    return true;
}

bool WorldSubsystem::CreateNewCharacter(CharacterCreationInfo CreationInfo,
                                        FPCore::World::GlobalCharacterID& OutNewCharacterID)
{
    // Find available ID
    for(OutNewCharacterID = 0; OutNewCharacterID < MaxCharacterCount; OutNewCharacterID++)
    {
        if (CharacterActiveFlags[OutNewCharacterID] == false)
        {
            break;
        }
    }

    if (OutNewCharacterID == MaxCharacterCount)
    {
        return false;
    }

    CharacterActiveFlags[OutNewCharacterID] = true;
    CharacterCoordinates[OutNewCharacterID] = CreationInfo.SpawnCoordinates;
    strcpy_s(CharacterNames[OutNewCharacterID], sizeof(FPCore::World::CharacterName), CreationInfo.Name);
    OnCharacterCreatedCallbackTable.TriggerCallbacks(OutNewCharacterID);

    std::cout << "Successfully created new Character ! ID = " << OutNewCharacterID << " | Name = '" << CharacterNames[OutNewCharacterID] << "'\n";
    
    return true;
}

void WorldSubsystem::OnClientAccountCreated(Client& NewClient, void* Context)
{
    WorldSubsystem& WorldSub = *static_cast<WorldSubsystem*>(Context);

    FPCore::World::GlobalCharacterID NewCharacterID;
    
    CharacterCreationInfo Info = {};
    strcpy_s(Info.Name, sizeof(Info.Name), NewClient.Account.UniqueUsername);
    Info.SpawnCoordinates = {5, 5};
    
    WorldSub.CreateNewCharacter(Info, NewCharacterID);
}
