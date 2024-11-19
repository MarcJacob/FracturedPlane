// ClientsSubsystem.h
// Contains Client / Player Account & Session management code

#pragma once

#include "cstdint"
#include "FPCore/Net/Packet.h"
#include "ServerFramework/Subsystems/Subsystem.h"

// DEPENDENCIES FORWARD DECLARATION
struct MemorySubsystem;
struct ConnectionsSubsystem;

struct Connection;

// Clients subsystem data types

typedef uint16_t ClientID_t;
static constexpr ClientID_t INVALID_CLIENT_ID = ~0;

#define CLIENT_USERNAME_MAX_LENGTH 32
typedef char Username_t[CLIENT_USERNAME_MAX_LENGTH];

// Core Account information linked to a Client and loaded / stored in persistent data.
struct AccountInfo
{
    Username_t UniqueUsername;
};

// Represents a specific Client known to the server, which may be currently connected or not. 
struct Client
{
    ClientID_t ID;  // Index of Client as it exists in Server storage.
    // Other systems may reference the Client through its ID aswell.
    // This is NOT tied to persistent Account data and thus NOT unique over time !
    AccountInfo Account;
    Connection* LinkedConnection; // Connection data linked to Client. Null if Client is offline.
};

namespace FPCore
{
    namespace Net
    {
        struct PacketData_Authentication;
    }
}

// EVENT TYPE DEFINITIONS.
// Registering to an event is done within specific arrays in the Subsystem, with pre-determined space.

// OnClientConnected: Called when a Client has come online (connected & authenticated)
typedef void (*OnClientConnectedFunc)(Client& AuthenticatedClient, void* Context);
// OnClientDisconnected: Called when a Client has gone offline.
typedef void (*OnClientDisconnectedFunc)(Client& DisconnectedClient, void* Context);
// OnClientAccountCreated: Called when a new Client account has been created.
typedef void (*OnClientAccountCreatedFunc)(Client& CreatedClient, void* Context);

struct ClientsSubsystem
{
    Client* Clients;
    size_t MaxClientCount;

    // Callback tables for Client connection and disconnection.
    // We support up to 8 callbacks.
    CallbackTable<OnClientConnectedFunc, 8> OnClientConnectedCallbackTable;
    CallbackTable<OnClientDisconnectedFunc, 8> OnClientDisconnectedCallbackTable;
    CallbackTable<OnClientAccountCreatedFunc, 8> OnClientAccountCreatedCallbackTable;
    
    // Linked Connections Subsystem, passed on initialization.
    ConnectionsSubsystem* ServerConnectionsSubsystem;
    
    // Initializes the Clients Subsystem, requiring a Memory subsystem to allocate the Clients buffer and a Connections
    // Subsystem to handle authentication and linking a Connection to a Client ID.
    bool Initialize(MemorySubsystem& Memory, size_t MaxClients, ConnectionsSubsystem& Connections);

    // Returns a Client using their account's Unique Username.
    // If no Client account with that name exists, returns null.
    Client* GetClientByUniqueUsername(Username_t Name);

    // Allocates and initializes a new Client account with the provided name, and returns it.
    // May fail and return null if there wasn't enough memory available.
    // #TODO(Marc): Need to store the bulk of Client accounts in some sort of persistent Database so that they don't
    // all need to be loaded at all times and can persist between server executions.
    Client* CreateNewClient(Username_t Name);

    // Attempts to Authenticate a Client based on the data passed in an Authentication Packet.
    // Returns true if successful. The passed Authentication Packet will be cleared and have its data
    // replaced with appropriate Response data so it can be sent back.
    bool ProcessAuthenticationRequest(Connection& RequestingConnection, FPCore::Net::PacketData_Authentication& AuthRequestPacket);

    // Packet Handlers
    
    static void HandleAuthenticationRequestPacket(FPCore::Net::Packet& Packet, void* Clients);
};
