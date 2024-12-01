#include "ServerFramework/Subsystems/Net/WorldSynchronizationSubsystem.h"

#include <iostream>

#include "ServerFramework/Subsystems/Core/ConnectionsSubsystem.h"
#include "ServerFramework/Subsystems/Core/MemorySubsystem.h"
#include "ServerFramework/Subsystems/Net/ClientsSubsystem.h"

#include "FPCore/World/World.h"
#include "ServerFramework/Subsystems/Core/WorldSubsystem.h"

#include "FPCore/Net/Packet/WorldSyncPackets.h"

bool WorldSynchronizationSubsystem::Initialize(MemorySubsystem& Memory, ClientsSubsystem& Clients, WorldSubsystem& World)
{
    LinkedClientsSubsystem = &Clients;
    LinkedWorldSubsystem = &World;

    MaxClientCount = Clients.MaxClientCount;
    ClientSyncStates = Memory.AllocateZeroed<ClientSyncState>(MaxClientCount);

    Clients.OnClientConnectedCallbackTable.RegisterCallback(OnClientConnected, this);
    Clients.OnClientDisconnectedCallbackTable.RegisterCallback(OnClientDisconnected, this);
    
    return ClientSyncStates != nullptr;
}

void WorldSynchronizationSubsystem::SyncClients()
{
    using namespace FPCore::World;
    
    // When running a full sync update, we assume that Client ID == Index of relevant Sync State.
    for(ClientID_t ClientID = 0; ClientID < MaxClientCount; ClientID++)
    {
        if (!ClientSyncStates[ClientID].bActive)
        {
            continue;
        }

        if (ClientSyncStates[ClientID].bRequiresZoneUpdate)
        {
            SynchronizeLandscape(LinkedClientsSubsystem->Clients[ClientID], ClientSyncStates[ClientID]);
            ClientSyncStates[ClientID].bRequiresZoneUpdate = false;
        }
    }
}

bool WorldSynchronizationSubsystem::SynchronizeLandscape(Client& ClientToSync, ClientSyncState& SyncState)
{
    // Synchronize Zone at 0,0 of Island 0 in Cluster 0.
    FPCore::Net::PacketBodyDef_ZoneLandscapeSync LandscapeSyncPacketData = {};
    LandscapeSyncPacketData.ZoneCoordinates = {0, 0};

    memcpy(LandscapeSyncPacketData.VoidTileBitflag, LinkedWorldSubsystem->IslandClusters[0].Islands[0].Zones[0].VoidTileBitMask, sizeof(LandscapeSyncPacketData.VoidTileBitflag));

    // Send full landscape data to Client's connection and return whether writing the packet for sending was a success.
    return LinkedClientsSubsystem->ServerConnectionsSubsystem->WriteOutgoingPacket(ClientToSync.LinkedConnection->ID, 
        FPCore::Net::PacketBodyType::WORLD_SYNC_LANDSCAPE, &LandscapeSyncPacketData);
}

void WorldSynchronizationSubsystem::OnClientConnected(Client& ConnectedClient, void* Context)
{
    WorldSynchronizationSubsystem& WorldSync = *static_cast<WorldSynchronizationSubsystem*>(Context);

    if (ConnectedClient.ID >= WorldSync.MaxClientCount)
    {
        std::cerr << "Error(WorldSynchronizationSubsystem): Invalid Client ID " << ConnectedClient.ID << " passed in OnClientConnected event handler !\n";
        return;
    }
    
    ClientSyncState& SyncState = WorldSync.ClientSyncStates[ConnectedClient.ID];

    if (SyncState.bActive)
    {
        std::cerr << "Warning(WorldSynchronizationSubsystem): Synchronization State ID " << ConnectedClient.ID << " is already in use!\n";
    }

    SyncState.bActive = true;
    SyncState.bRequiresZoneUpdate = true;
}

void WorldSynchronizationSubsystem::OnClientDisconnected(Client& DisconnectedClient, void* Context)
{
    WorldSynchronizationSubsystem& WorldSync = *static_cast<WorldSynchronizationSubsystem*>(Context);

    if (DisconnectedClient.ID >= WorldSync.MaxClientCount)
    {
        std::cerr << "Error(WorldSynchronizationSubsystem): Invalid Client ID " << DisconnectedClient.ID << " passed in OnClientDisconnected event handler !\n";
        return;
    }
    
    ClientSyncState& SyncState = WorldSync.ClientSyncStates[DisconnectedClient.ID];

    if (!SyncState.bActive)
    {
        std::cerr << "Warning(WorldSynchronizationSubsystem): Synchronization State ID " << DisconnectedClient.ID << " was not in use !\n";
    }

    SyncState.bActive = false;
}
