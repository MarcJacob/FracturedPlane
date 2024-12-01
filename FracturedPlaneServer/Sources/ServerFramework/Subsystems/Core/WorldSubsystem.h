// WorldSubsystem.h
// Server Subsystem in charge of managing World Data, running world real-time simulation and accepting external events
// that influence the world.

#pragma once

#include "FPCore/World/World.h"
#include "ServerFramework/Subsystems/Subsystem.h"
#include "Math/Math.h"

// EXTERNAL DEPENDENCIES FORWARD DECLARATION
struct MemorySubsystem;
struct Client;

struct CharacterCreationInfo
{
    char Name[32];
    FPCore::World::Coordinates SpawnCoordinates;
};

// Collection of Islands within interaction range.
struct Cluster
{
    FPCore::World::ClusterID ID;
    
    struct Island
    {
        bool bActive; // Active flag. If false the Island is available for generation, placement and updating within the Cluster.

        FPCore::World::ClusterID ClusterID; // ID of the Cluster this island is part of.
        FPCore::World::IslandID ID; // Unique identifier for this island within its Cluster.

        Vec2<uint16_t> Position; // Position of the North-Western Corner of the island (zone coordinates [0, 0]).
        Vec2<uint16_t> Bounds; // Rectangular bounds of the island encapsulating all of its zones.

        FPCore::World::ZoneDef* Zones; // Contains all zones in a contiguous sequence. TODO: Eliminate void zones from this array.
        size_t ZoneCount;
    };

    // Islands buffer.
    // The Cluster can be sized for a specific number of islands. 
    Island* Islands;
    int IslandSlotCount;
};

// All data related to the act of generating a new island.
// Every field can be pre-set as a hint before 
struct IslandGenerationInfo
{
    FPCore::World::ClusterID ClusterID = ~0; // Cluster within which the generated Island now exists.

    FPCore::World::IslandID ID; // ID of the generated island, unique within its Cluster. Ignored if passed as hint.

    Vec2<uint16_t> BoundsSize;       // Islands have rectangular bounds including all of their zones. Required parameter.

    size_t ZoneCount = 0;   // Total number of non-void zones in the generated island. If passed as hint, will be interpreted as maximum / target zone count.
    // Hence final Island zone density should be at most ZoneCount / BoundsSize.X * BoundsSize.Y.

// TODO Other island generation parameters / info (elevation min / max, temperature min / max...)
};

// Subsystem in charge of managing memory for World Elements and Simulating the world in real time, as well
// as receiving various external events that drive the simulation according to player or AI input.
struct WorldSubsystem
{
    bool bWorldGenerated = false;
    
    Cluster IslandClusters[8];

    bool Initialize(MemorySubsystem& Memory);

    // Generates a new island in an automatically chosen Cluster. Returns whether the operation was a success.
    // Some parameters in the Generation Info structure have to be passed for generation to succeed
    // Other properties of the Generation Info structure can be used as hints, but never requirements.
    // (See IslandGenerationInfo structure comments !)
    // If anything passed as hint is required for valid generation, it should be checked afterwards, and the island deleted if need be.
    // Memory is allocated as required to generate an island of appropriate size.
    bool GenerateIsland(MemorySubsystem& Memory, IslandGenerationInfo& GenInfo);
    // Deletes an Island from the world entirely. No event is tied to this call, so this should be done as the last step of any
    // mechanic leading to the destruction of the island !
    void DeleteIsland(FPCore::World::ClusterID, FPCore::World::IslandID); 

    // Attempts to create a new character using the passed info and places them into the world.
    // If successful, the OutNewCharacterID out parameter will be populated with the new character's Global ID,
    // which can be used to read its data in the various Character buffers.
    bool CreateNewCharacter(CharacterCreationInfo CreationInfo, FPCore::World::Entities::CharacterID& OutNewCharacterID);

    // Event handlers
    static void OnClientAccountCreated(Client& NewClient, void* Context);
};