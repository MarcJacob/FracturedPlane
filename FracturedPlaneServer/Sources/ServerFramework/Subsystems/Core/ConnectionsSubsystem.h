// ServerConnections.h
// Contains code related to managing Connections on the Master Server (to Clients, sub-servers, databases...)
// And Client Data.

#pragma once

#include <cstdint>
#include <mutex>

#include "ServerFramework/ServerPlatform.h"

// DEPENDENCIES FORWARD DECLARATION
struct MemorySubsystem;
struct Client;

typedef uint16_t ServerConnectionID_t;
static constexpr ServerConnectionID_t INVALID_CONNECTION_ID = ~0;

// Contains data about an Active Connection to the Server, possibly linking to a Client.
// Manages data reception & sending through those connections.
struct Connection
{
    ServerConnectionID_t ID;
    ServerPlatform::ConnectionID PlatformConnectionID;
    Client* LinkedClient; // When Authenticated, a Connection will be linked to a loaded Client which
    // will mark it as being Online and will allow the proper reception & processing of many packet types.
    // TODO(Marc): This should be stored in a map located on the Clients subsystem instead.
    // Doing it this way is conceptually better - Connections are used as bridges to the Platform Sockets,
    // and whatever we want to associate to them externally should be done fully externally.
    
    float ConnectionUpTime; // How long has this connection been active.
    float LastReceptionTime; // How long has this connection not sent a message for.
};

typedef void (*NetPacketReceptionHandlerFunc)(FPCore::Net::Packet& Packet, void* Context);
struct RegisteredPacketReceptionHandler
{
    void* Context;
    NetPacketReceptionHandlerFunc HandlerFunc;
};

// Wraps an internal table linking every message type to up to one handler function.
struct NetPacketReceptionTable_t
{
    void AssignHandler(FPCore::Net::PacketType PacketType, NetPacketReceptionHandlerFunc Handler, void* Context = nullptr);
    void HandlePacket(FPCore::Net::Packet& Packet);
    
    RegisteredPacketReceptionHandler PacketReceptionHandlers[static_cast<int>(FPCore::Net::PacketType::PACKET_TYPE_COUNT)];
};

// Server Management Object handling Connections of all kinds.
struct ConnectionsSubsystem
{
    Connection* ActiveConnections;
    size_t MaxConnectionCount;

    // Links Packet Types to a specific handler function to be called if any.
    NetPacketReceptionTable_t PacketReceptionTable;
    // Internal buffer regularly flushed to the Platform Sending buffer.
    struct
    {
        byte* WriteBuffer;
        size_t WriteBufferSize;
        size_t WrittenBytes; // Number of bytes written since last flush.
    } PacketWriter;
    
    // Initializes the Connections Subsystem, requiring a Memory subsystem to allocate the Active Connections buffer
    // for the specified number of maximum connections we want to handle at once, aswell as a Packet Reception Table
    // so the subsystem may handle authentication request packets.
    bool Initialize(MemorySubsystem& Memory, size_t MaxConnection);

    // Updates lifetime data on all active connections.
    // #TODO(Marc): Should run heartbeats / auto disconnects once subsystems acquire the ability to request the direct
    // or indirect use of Platform Net functions.
    void UpdateConnections(float UpdateDeltaTime);
    
    Connection* RegisterConnection(ServerPlatform::ConnectionID ConnectedSocketID);
    void DeleteConnection(ServerConnectionID_t Connection);

    // Establishes a one-way link between a Client and a Connection.
    void ConnectClient(ServerConnectionID_t ConnectionID, Client* ClientToConnect);
    
    Connection* GetConnectionFromPlatformSocket(ServerPlatform::ConnectionID SocketID);

    void HandleIncomingPacket(FPCore::Net::Packet& Packet);
    
    // Writes a packet to be sent to the passed Connection ID into the buffer. Requires successful locking of the
    // internal Write Mutex.
    // Returns whether writing was successful.
    // NOTE: The passed Packet's entire data is copied into the sending buffer, meaning it is safe to free the Packet
    // after writing.
    bool WriteOutgoingPacket(const FPCore::Net::Packet& Packet);
    
    // Flushes the internal packed sending buffer to the platform sending buffer, performing appropriate checks and
    // conversions. Returns how many bytes were written.
    size_t FlushSendingBufferToPlatformBuffer(byte* PlatformWriteBuffer, size_t PlatformWriteBufferSize);
};