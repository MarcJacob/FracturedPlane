#include "ServerFramework/Subsystems/Net/ClientsSubsystem.h"

#include <cstring>
#include <iostream>

#include "FPCore/Net/Packet/AuthenticationPackets.h"
#include "ServerFramework/Subsystems/Core/ConnectionsSubsystem.h"
#include "ServerFramework/Subsystems/Core/MemorySubsystem.h"

bool ClientsSubsystem::Initialize(MemorySubsystem& Memory, size_t MaxClients, ConnectionsSubsystem& Connections)
{
    // Allocate Client buffer.
    MaxClientCount = MaxClients;
    Clients = static_cast<Client*>(Memory.Allocate(MaxClients * sizeof(Client)));

    if (nullptr == Clients)
    {
        return false;
    }
    
    memset(Clients, 0, MaxClients * sizeof(Client));

    for(ClientID_t ClientID = 0; ClientID < MaxClientCount; ++ClientID)
    {
        Clients[ClientID].ID = INVALID_CLIENT_ID;
    }

    // Link Connections Subsystem
    ServerConnectionsSubsystem = &Connections;

    ServerConnectionsSubsystem->PacketReceptionTable.AssignHandler(FPCore::Net::PacketBodyType::AUTHENTICATION, HandleAuthenticationRequestPacket, this);

    OnClientAccountCreatedCallbackTable = {0};
    OnClientConnectedCallbackTable = {0};
    OnClientDisconnectedCallbackTable = {0};
    
    return true;
}

Client* ClientsSubsystem::GetClientByUniqueUsername(Username_t Name)
{
    for(ClientID_t ClientID = 0; ClientID < MaxClientCount; ++ClientID)
    {
        if (strcmp(Clients[ClientID].Account.UniqueUsername, Name) == 0)
        {
            return &Clients[ClientID];
        }
    }

    return nullptr;
}

Client* ClientsSubsystem::CreateNewClient(Username_t Name)
{
    for(ClientID_t ClientID = 0; ClientID < MaxClientCount; ++ClientID)
    {
        if (Clients[ClientID].ID == INVALID_CLIENT_ID)
        {
            // Allocate new Client and return its address.
            Clients[ClientID].ID = ClientID;
            Clients[ClientID].LinkedConnection = nullptr;

            Clients[ClientID].Account = {};
            strcpy_s(Clients[ClientID].Account.UniqueUsername, CLIENT_USERNAME_MAX_LENGTH, Name);
    
            OnClientAccountCreatedCallbackTable.TriggerCallbacks(Clients[ClientID]);
            
            return &Clients[ClientID];
        }
    }

    std::cerr << "ERROR (Clients Subsystem): Max amount of Clients reached !\n";
    return nullptr;
}

bool ClientsSubsystem::ProcessAuthenticationRequest(Connection& RequestingConnection, FPCore::Net::PacketBodyDef_Authentication& AuthRequestPacket)
{
    Client* AuthenticatedClient = nullptr;
    
    // Determine Username validity.
    bool UsernameIsValid = true;
    {
        int UsernameLength = strnlen_s(AuthRequestPacket.Request.Username, sizeof(Username_t));
        if (UsernameLength < 3)
        {
            UsernameIsValid = false;
        }
    }

    if (!UsernameIsValid)
    {
        std::cerr << "Error: Could not Authenticate Connection ID " << RequestingConnection.ID << ": Invalid Username.\n";
        goto End;
    }
    
    // Determine if new Client has to be created or if it already exists.
    // #TODO(Marc): Creating a new account should be done separately.
    AuthenticatedClient = GetClientByUniqueUsername(AuthRequestPacket.Request.Username);
    
    if (nullptr == AuthenticatedClient)
    {
        // Client doesn't exist yet. Create a new one with the provided name.
        Username_t NewClientUsername;
        strcpy_s(NewClientUsername, sizeof(NewClientUsername), AuthRequestPacket.Request.Username);

        AuthenticatedClient = CreateNewClient(NewClientUsername);
    }
    else
    {
        // Check that Client can be authenticated (not banned, not already authenticated...)
    }

    // At that point, the absence of an Authenticated Client means the server is unable to accept authentication for some
    // reason.
    if (nullptr == AuthenticatedClient)
    {
        std::cerr << "Client authentication failed. The Client could not be found or created on the server. Request Connection ID = " << RequestingConnection.ID << " !\n"
        << "Denying authentication request.";
    }
    else
    {
        // Link Connection to Authenticated Client.
        ServerConnectionsSubsystem->ConnectClient(RequestingConnection.ID, AuthenticatedClient);

        if (RequestingConnection.LinkedClient != AuthenticatedClient)
        {
            std::cerr << "Client Authentication failed. The Client might already be tied to another Connection.\n";
            AuthenticatedClient = nullptr;
        }
        else
        {
            std::cout << "Authenticated Client ID " << AuthenticatedClient->ID << " as User '" << AuthenticatedClient->Account.UniqueUsername << "'.\n";
            AuthenticatedClient->LinkedConnection = &RequestingConnection;

            OnClientConnectedCallbackTable.TriggerCallbacks(*AuthenticatedClient);
        }
    }

    End:
    // Clear existing data on the Request packet.
    AuthRequestPacket = {};
    AuthRequestPacket.Response.bAccepted = nullptr != AuthenticatedClient;
    return nullptr != AuthenticatedClient;
}

// Context = Clients Subsystem
void ClientsSubsystem::HandleAuthenticationRequestPacket(FPCore::Net::PacketHead& AuthPacket, void* Context)
{
    ClientsSubsystem* Clients = static_cast<ClientsSubsystem*>(Context);
    
    // Process Authentication Request.

    // Read Packet data as a Authentication Request Packet. This same data will be read by the authentication system
    // and have its data replaced so it can be sent back as a response.
    FPCore::Net::PacketBodyDef_Authentication AuthPacketData = AuthPacket.ReadBodyDef<
        FPCore::Net::PacketBodyDef_Authentication>();

    Connection& ReceptionConnection = Clients->ServerConnectionsSubsystem->ActiveConnections[AuthPacket.ConnectionID];
    
    // Run Authentication Process. Return result does not interest us as the Packet will contain the appropriate response data already.
    Clients->ProcessAuthenticationRequest(ReceptionConnection, AuthPacketData);

    // Send Auth Response back.
    Clients->ServerConnectionsSubsystem->WriteOutgoingPacket(ReceptionConnection.ID, FPCore::Net::PacketBodyType::AUTHENTICATION, &AuthPacketData);
}

