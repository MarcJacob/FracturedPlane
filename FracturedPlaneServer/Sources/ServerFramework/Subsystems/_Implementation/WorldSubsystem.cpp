#include "ServerFramework/Subsystems/Core/WorldSubsystem.h"
#include "ServerFramework/Subsystems/Core/MemorySubsystem.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

#include "ServerFramework/Subsystems/Net/ClientsSubsystem.h"

bool WorldSubsystem::Initialize(MemorySubsystem& Memory)
{
    // Create a single Cluster with 16 Island slots.
    IslandClusters[0].ID = 0;
    IslandClusters[0].Islands = Memory.AllocateAndInit<Cluster::Island>(16);
    IslandClusters[0].IslandSlotCount = 16;

    return true;
}

bool WorldSubsystem::GenerateIsland(MemorySubsystem& Memory, IslandGenerationInfo& GenInfo)
{
    // Prime Randomizer (TODO: Find a proper random library)
    srand(time(NULL));

    // Find available spot for a new Island among the existing Clusters.
    bool SpotFound = false;

    // If a Cluster ID was provided as a hint, attempt to use it.
    if (GenInfo.ClusterID != ~0)
    {
        Cluster& Cluster = IslandClusters[GenInfo.ClusterID];
        for (FPCore::World::IslandID IslandIndex = 0; IslandIndex < Cluster.IslandSlotCount; IslandIndex++)
        {
            if (Cluster.Islands[IslandIndex].bActive == false)
            {
                GenInfo.ID = IslandIndex;
                SpotFound = true;
                break;
            }
        }
    }
    
    if (!SpotFound)
    {
        for (FPCore::World::ClusterID ClusterIndex = 0; ClusterIndex < 8; ClusterIndex++)
        {
            Cluster& Cluster = IslandClusters[ClusterIndex];
            for (FPCore::World::IslandID IslandIndex = 0; IslandIndex < Cluster.IslandSlotCount; IslandIndex++)
            {
                if (Cluster.Islands[IslandIndex].bActive == false)
                {
                    GenInfo.ClusterID = ClusterIndex;
                    GenInfo.ID = IslandIndex;
                    SpotFound = true;
                    break;
                }
            }
            if (SpotFound)
            {
                break;
            }
        }
    }
    
    if (!SpotFound)
    {
        return false;
    }

    Cluster& ChosenCluster = IslandClusters[GenInfo.ClusterID];

    // Create new Island within the chosen spot.
    
    Cluster::Island& NewIsland = ChosenCluster.Islands[GenInfo.ID];

    NewIsland.bActive = true;
    NewIsland.ID = GenInfo.ID;
    NewIsland.ClusterID = ChosenCluster.ID;
    NewIsland.Position = { static_cast<unsigned short>(rand() % 2000), static_cast<unsigned short>(rand() % 2000) };
    NewIsland.Bounds = GenInfo.BoundsSize;

    // Allocate zones
    NewIsland.Zones = Memory.AllocateZeroed<FPCore::World::ZoneDef>(GenInfo.BoundsSize.X * GenInfo.BoundsSize.Y);
    NewIsland.ZoneCount = GenInfo.ZoneCount;

    // Generate zone 0,0 at random TODO: Zone generation should only work if the zone is "opened" by founding a site or a mission takes place there.
    FPCore::World::Coordinates TileCoords = { 0, 0 };
    float RandFactor = static_cast<float>(rand());
    for (TileCoords.X = 0; TileCoords.X < FPCore::World::ZONE_SIZE_TILES; TileCoords.X++)
    {
        for (TileCoords.Y = 0; TileCoords.Y < FPCore::World::ZONE_SIZE_TILES; TileCoords.Y++)
        {
            float Epicness = 1.f;
            if (Epicness > 0.f)
            {
                size_t BitIndex = TileCoords.X * NewIsland.Bounds.Y + TileCoords.Y;
                NewIsland.Zones[0].VoidTileBitMask[BitIndex / 8] += 1 << (BitIndex % 8);
            }
        }
    }

    // Update Gen Info data
    GenInfo.ZoneCount = NewIsland.ZoneCount;
    GenInfo.BoundsSize = NewIsland.Bounds;

    return true;
}

void WorldSubsystem::DeleteIsland(FPCore::World::ClusterID, FPCore::World::IslandID)
{
}

bool WorldSubsystem::CreateNewCharacter(CharacterCreationInfo CreationInfo, FPCore::World::Entities::CharacterID& OutNewCharacterID)
{
    // TODO: Character creation.
    return false;
}

void WorldSubsystem::OnClientAccountCreated(Client& NewClient, void* Context)
{
    WorldSubsystem& WorldSub = *static_cast<WorldSubsystem*>(Context);

    FPCore::World::Entities::CharacterID NewCharacterID;
    
    CharacterCreationInfo Info = {};
    strcpy_s(Info.Name, sizeof(Info.Name), NewClient.Account.UniqueUsername);
    Info.SpawnCoordinates = {5, 5};
    
    WorldSub.CreateNewCharacter(Info, NewCharacterID);
}
