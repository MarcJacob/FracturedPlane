// WorldSynchronizationSubsystem.h
// Contains necessary declarations for the World Synchronization Subsystem.

#pragma once
#include "FPCore/World/World.h"

// DEPENDENCIES FORWARD DECLARATION
struct MemorySubsystem;
struct ClientsSubsystem;
struct Client;

struct WorldSubsystem;

struct ClientSyncState
{
    // Is this Sync State relevant to any online client ?
    bool bActive;
    // Has the client been given the Landscape data ?
    bool bRequiresLandscapeDataSync;
    // Should we send this client a full Entities update ?
    bool bRequiresEntitiesSync;
};

// Subsystem tasked with handling World Data synchronization for clients.
// Does so by maintaining a copy of relevant World Data in a double-buffer fashion and determining
// which client requires which data.
struct WorldSynchronizationSubsystem
{
    ClientSyncState* ClientSyncStates;
    size_t MaxClientCount;

    ClientsSubsystem* LinkedClientsSubsystem;
    WorldSubsystem* LinkedWorldSubsystem;

    // Initialize this subsystem. Requires a Clients and World Subsystem to link to. Requires a Memory Subsystem to allocate
    // buffers, whose size will depend on Clients Subsystem max supported clients.
    bool Initialize(MemorySubsystem& Memory, ClientsSubsystem& Clients, WorldSubsystem& World,
        size_t SyncMapCount);

    void SyncClients();

    bool SynchronizeLandscape(Client& ClientToSync, ClientSyncState& SyncState);
    
    // Handler for On Client Connected event in Clients Subsystem.
    // Context = pointer to this structure.
    static void OnClientConnected(Client& ConnectedClient, void* Context);

    // Handler for On Client Disconnected event in Clients Subsystem.
    // Context = pointer to this structure.
    static void OnClientDisconnected(Client& DisconnectedClient, void* Context);
};