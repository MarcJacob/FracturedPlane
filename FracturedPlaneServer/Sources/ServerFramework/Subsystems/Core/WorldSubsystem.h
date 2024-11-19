// WorldSubsystem.h
// Server Subsystem in charge of managing World Data, running world real-time simulation and accepting external events
// that influence the world.

#pragma once

#include "FPCore/World/World.h"
#include "ServerFramework/Subsystems/Subsystem.h"

// EXTERNAL DEPENDENCIES FORWARD DECLARATION
struct MemorySubsystem;
struct Client;

typedef uint32_t WorldSize_t;

struct CharacterCreationInfo
{
    char Name[32];
    FPCore::World::Coordinates SpawnCoordinates;
};

typedef void (*OnCharacterCreatedFunc)(FPCore::World::GlobalCharacterID& CreatedCharacterID, void* Context); 

// Subsystem in charge of managing memory for World Elements and Simulating the world in real time, aswell
// as receiving various external events that drive the simulation according to player or AI input.
struct WorldSubsystem
{
    bool bWorldGenerated = false;

    CallbackTable<OnCharacterCreatedFunc, 8> OnCharacterCreatedCallbackTable;
    
    // The world is square, its size being the length of one side.
    // #TODO(Marc): It would be smart to cut it down to Regions so larger expenses of void wouldn't have to be
    // stored in memory.
    WorldSize_t WorldSize;
    
    // Contains all data related to the world's tiles.
    FPCore::World::Landscape        WorldLandscape;

    // Character Data. Character ID maps to these buffers.

    size_t MaxCharacterCount;
    // Defines whether the Character exists.
    bool*                           CharacterActiveFlags;
    // Defines the character World coordinates.
    FPCore::World::Coordinates*     CharacterCoordinates;
    // Defines the character Names.
    FPCore::World::CharacterName*   CharacterNames;
    
    // (Re)generates the World's landscape.
    bool GenerateWorldLandscape(MemorySubsystem& Memory, WorldSize_t Size);

    bool InitializeEntityData(MemorySubsystem& Memory, size_t CharacterCount);
    
    // Attempts to create a new character using the passed info and places them into the world.
    // If successful, the OutNewCharacterID out parameter will be populated with the new character's Global ID,
    // which can be used to read its data in the various Character buffers.
    bool CreateNewCharacter(CharacterCreationInfo CreationInfo, FPCore::World::GlobalCharacterID& OutNewCharacterID);

    // Event handlers
    static void OnClientAccountCreated(Client& NewClient, void* Context);
};