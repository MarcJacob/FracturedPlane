// Server_Main.cpp
// Main entry points for Server execution, called by Platform main thread.

#include "FPCore/Net/Packet.h"
#include "ServerPlatform.h"
#include "Server.h"
#include "iostream"

void NetPacketReceptionTable_t::AssignHandler(FPCore::Net::PacketType PacketType, NetPacketReceptionHandlerFunc Handler, void* Context)
{
    int PacketTypeIndex = static_cast<int>(PacketType);
    if (PacketTypeIndex < 0 || PacketType >= FPCore::Net::PacketType::PACKET_TYPE_COUNT)
    {
        std::cerr << "Error: Attempted to assign a handler to an invalid Packet Type value !\n";
        return;
    }
    
    if (PacketReceptionHandlers[PacketTypeIndex].HandlerFunc != nullptr)
    {
        std::cerr << "Error: Attempted to assign more than one Handler for Packet Type " << static_cast<int>(PacketType) << ".\n";
        return;
    }

    PacketReceptionHandlers[PacketTypeIndex] = { Context, Handler };
}

void NetPacketReceptionTable_t::HandlePacket(FPCore::Net::Packet& Packet)
{
    int PacketTypeIndex = static_cast<int>(Packet.Type);

    if (PacketTypeIndex >= 0 && PacketTypeIndex < static_cast<int>(FPCore::Net::PacketType::PACKET_TYPE_COUNT))
    {
        if (PacketReceptionHandlers[PacketTypeIndex].HandlerFunc != nullptr)
        {
            PacketReceptionHandlers[PacketTypeIndex].HandlerFunc(Packet, PacketReceptionHandlers[PacketTypeIndex].Context);
        }
    }
}


size_t GetRequiredServerMemory()
{
    // Ask for enough memory to hold Server State Data + 64mb
    return sizeof(ServerStateData) + static_cast<size_t>(1024 * 1024 * 64);
}

// PLATFORM CALL
// Initializes a new Game Server from the passed Platform's memory and returns it in the form of void pointer through the OutGameServerPtr parameter.
// The pointer returned here will be used in follow up Update and Shutdown calls. This allows complete separation between Server and Platform code outside the
// major Flow functions.
bool InitializeServer(const ServerPlatform& Platform, GameServerPtr& OutGameServerPtr)
{
    // Check Platform validity
    if (Platform.Memory == nullptr || Platform.MemorySize == 0 || Platform.MemorySize < GetRequiredServerMemory())
    {
        std::cout << "SERVER INITIALIZATION FAILED: Platform did not allocate sufficient memory.\n";
        return false;
    }
    
    // Make sure allocated memory is entirely zeroed-out
    memset(Platform.Memory, 0, Platform.MemorySize);

    // Interpret the beginning of platform memory as the Server State Data.
    ServerStateData* OutGameServer = reinterpret_cast<ServerStateData*>(Platform.Memory);
    
    // Allocate Server State & Subsystems at the start of allocated memory.
    *OutGameServer = ServerStateData();

    OutGameServer->ServerUp = true;
    OutGameServer->Platform = &Platform;
    
    // Load critical Server Data & Config
    // ...

    // Initialize Server Subsystems

    // Initialize Memory Subsystem
    // The memory it is given to manage is all of the Platform's allocated memory, minus the size of the Server State Data structure since those bytes have been used for it.
    if (!OutGameServer->Memory.Initialize(Platform.Memory + sizeof(ServerStateData), Platform.MemorySize - sizeof(ServerStateData)))
    {
        std::cerr << "Fatal Error when initializing Server: Couldn't initialize Memory Subsystem !\n";
        return false;
    }

    // Initialize Other Subsystems in order of dependencies.
    if (!OutGameServer->Connections.Initialize(OutGameServer->Memory, 256))
    {
        std::cerr << "Fatal Error when initializing Server: Couldn't initialize Connections Subsystem.\n";
        return false;
    }

    if (!OutGameServer->World.GenerateWorldLandscape(OutGameServer->Memory, 32))
    {
        std::cerr << "Fatal Error when initializing Server: Couldn't generate World Landscape !\n";
        return false;
    }

    if (!OutGameServer->World.InitializeEntityData(OutGameServer->Memory, 32))
    {
        std::cerr << "Fatal Error when initializing Server: Couldn't initialize World Entity Data !\n";
        return false;
    }
    
    if (!OutGameServer->Clients.Initialize(OutGameServer->Memory, 128, OutGameServer->Connections))
    {
        std::cerr << "Fatal Error when initializing Server: Couldn't initialize Clients Subsystem.\n";
        return false;
    }

    OutGameServer->Clients.OnClientAccountCreatedCallbackTable.RegisterCallback(WorldSubsystem::OnClientAccountCreated, &OutGameServer->World);
    
    if (!OutGameServer->WorldSynchronization.Initialize(OutGameServer->Memory, OutGameServer->Clients, OutGameServer->World, 1))
    {
        std::cerr << "Fatal Error when initializing Server: Couldn't initialize World Synchronization Subsystem.\n";
        return false;
    }

    OutGameServerPtr = OutGameServer;
    
    std::cout << "Server initialization successful.\n";
    return true;
}

// Process time passage on the server, being passed the time that has passed since the beginning of the previous update.
void UpdateServer(GameServerPtr ServerPtr, const double& DeltaTime)
{
    ServerStateData& Server = *static_cast<ServerStateData*>(ServerPtr);
    Server.UptimeSeconds += DeltaTime;

    // Read Net Events (Connections and Disconnections)
    {
        const ServerPlatform::ConnectionID* ConnectedSocketIDs;
        size_t ConnectedCount;

        const ServerPlatform::ConnectionID* DisconnectedSocketIDs;
        size_t DisconnectedCount;

        Server.Platform->ReadPlatformNetEvents(ConnectedSocketIDs, ConnectedCount, DisconnectedSocketIDs, DisconnectedCount);

        // CONNECTIONS
        if (ConnectedCount > 0)
        {
            for (int i = 0 ; i < ConnectedCount ; i++)
            {
                ServerPlatform::ConnectionID SocketID = ConnectedSocketIDs[i];
                if (SocketID == ServerPlatform::INVALID_ID)
                {
                    continue;
                }
                
                Connection* NewConnection = Server.Connections.RegisterConnection(SocketID);
                if (NewConnection == nullptr)
                {
                    std::cerr << "Failed to create a new Connection ! Socket handle: " << SocketID << "\n";
                    Server.Platform->CloseConnection(SocketID);
                    continue;
                }

                std::cout << "Registered Server Connection ID " << NewConnection->ID << " with Socket ID " << SocketID << "\n";
            }
        }

        // DISCONNECTIONS
        if (DisconnectedCount > 0)
        {
            for (int i = 0 ; i < DisconnectedCount ; i++)
            {
                ServerPlatform::ConnectionID SocketID = DisconnectedSocketIDs[i];
                Connection* ServerConnection = Server.Connections.GetConnectionFromPlatformSocket(SocketID);
                if (nullptr == ServerConnection)
                {
                    // This can happen if the connection was closed by the server, in which case the associated connection data will have been deleted
                    // already.
                    continue;
                }
                
                std::cout << "Acknowledging Closing of Server Connection ID " << ServerConnection->ID << " with Platform Socket ID " << SocketID << "\n";

                // If Connection was linked to a Client, set the Client as offline while displaying their info.
                if (ServerConnection->LinkedClient != nullptr)
                {
                    std::cout << "Client Account Info:\n\tName: " << ServerConnection->LinkedClient->Account.UniqueUsername << "\n";

                    // #TODO(Marc): Move to a function in the Clients Subsystem.
                    ServerConnection->LinkedClient->LinkedConnection = nullptr;
                    Server.Clients.OnClientDisconnectedCallbackTable.TriggerCallbacks(*ServerConnection->LinkedClient);
                }
                Server.Connections.DeleteConnection(ServerConnection->ID);
            }
            
        }

        Server.Platform->ReleasePlatformNetEvents();
    }

    // Read Net Data Reception
    {
        const byte* PlatformNetReceptionBuffer;
        size_t ReceivedByteCount;

        Server.Platform->ReadPlatformNetReceptionBuffer(PlatformNetReceptionBuffer, ReceivedByteCount);
        if (ReceivedByteCount > 0)
        {
            const byte* ReceptionBufferEnd = PlatformNetReceptionBuffer + ReceivedByteCount;

            // Read entire buffer as packets sequentially, each time first extracting the Connection ID then extracting the
            // Packet itself.
            const byte* NextPacketAddress = PlatformNetReceptionBuffer;

            do
            {
                // Decode next packet.
                FPCore::Net::Packet ReceivedPacket = {};
                NextPacketAddress = GetNextPacketFromBuffer(NextPacketAddress, ReceptionBufferEnd, ReceivedPacket);
                if (NextPacketAddress == nullptr)
                {
                    break;
                }

                switch (ReceivedPacket.Type)
                {
                case(FPCore::Net::PacketType::MESSAGE):
                    // Log received message.
                    std::cout << "Received Message from Connection ID " << ReceivedPacket.ConnectionID << " :'" << static_cast<
                        const char*>(ReceivedPacket.Data) << "'\n";
                    break;
                case(FPCore::Net::PacketType::INVALID):
                    std::cerr << "Invalid packet type has been received from Connection ID " << ReceivedPacket.ConnectionID <<
                        "! Aborting reception loop.\n";
                    break;
                default:
                    Server.Connections.HandleIncomingPacket(ReceivedPacket);
                }
            }
            while (NextPacketAddress < ReceptionBufferEnd);

            if (nullptr == NextPacketAddress)
            {
                std::cerr << "Error when decoding received packets on Game Server !\n";
            }
        }
        Server.Platform->ReleasePlatformNetReceptionBuffer();
    }

    // Update Connections & Eliminate non-authenticated connections that have been active for too long.
    {
        Server.Connections.UpdateConnections(DeltaTime);

        for(ServerConnectionID_t ConnectionID = 0; ConnectionID < Server.Connections.MaxConnectionCount; ConnectionID++)
        {
            Connection& Conn = Server.Connections.ActiveConnections[ConnectionID]; 
            if (Conn.PlatformConnectionID == ServerPlatform::INVALID_ID)
            {
                continue;
            }

            if (Conn.LinkedClient == nullptr && Conn.ConnectionUpTime > 10.f)
            {
                std::cout << "Connection ID " << Conn.ID << " took too long to authenticate. Closing.\n";
                Server.Platform->CloseConnection(Conn.PlatformConnectionID);
                Server.Connections.DeleteConnection(ConnectionID);
            }
        }
    }

    // Update World & World Synchronization.
    {
        Server.WorldSynchronization.SyncClients();
    }
    
    // Flush Connections Subsystem's Packet Writer and fill in the Platform Sending Buffer for sending.
    if (Server.Connections.PacketWriter.WrittenBytes > 0)
    {
        byte* OutSendingBuffer;
        size_t OutSendingByteCount;
        Server.Platform->WriteToPlatformNetSendingBuffer(OutSendingBuffer, OutSendingByteCount);

        memcpy(OutSendingBuffer, Server.Connections.PacketWriter.WriteBuffer, Server.Connections.PacketWriter.WrittenBytes);
        
        Server.Platform->ReleasePlatformNetSendingBuffer(Server.Connections.PacketWriter.WrittenBytes);
        Server.Connections.PacketWriter.WrittenBytes = 0;
    }
}

void ShutdownServer(GameServerPtr ServerPtr, const ShutdownReason& Platform)
{
    ServerStateData& Server = *static_cast<ServerStateData*>(ServerPtr);
    Server.ServerUp = false;

    // Attempt to save Game World.
    // ...
    
    // Cleanup server subsystems
    Server.Memory.FreeServerHeap();
}