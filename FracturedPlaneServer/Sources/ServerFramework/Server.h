// ServerState.h
// Declares core server state data and functionality usable by all subsystems.

#pragma once

#include "Subsystems/Core/MemorySubsystem.h"
#include "Subsystems/Core/ConnectionsSubsystem.h"
#include "Subsystems/Core/WorldSubsystem.h"

#include "Subsystems/Net/ClientsSubsystem.h"
#include "Subsystems/Net/WorldSynchronizationSubsystem.h"

struct ServerStateData
{
    float UptimeSeconds = 0.f;
    bool ServerUp = false;

    const ServerPlatform* Platform = nullptr;

    // Server Subsystems
    MemorySubsystem Memory;
    ConnectionsSubsystem Connections;
    WorldSubsystem World;

    ClientsSubsystem Clients;
    WorldSynchronizationSubsystem WorldSynchronization;
};