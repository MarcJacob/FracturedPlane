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

typedef void (*NetPacketReceptionHandlerFunc)(FPCore::Net::PacketHead& Packet, void* Context);
struct RegisteredPacketReceptionHandler
{
    void* Context;
    NetPacketReceptionHandlerFunc HandlerFunc;
};

// Wraps an internal table linking every message type to up to one handler function.
struct NetPacketReceptionTable_t
{
    void AssignHandler(FPCore::Net::PacketBodyType PacketType, NetPacketReceptionHandlerFunc Handler, void* Context = nullptr);
    void HandlePacket(FPCore::Net::PacketHead& Packet);
    
    RegisteredPacketReceptionHandler PacketReceptionHandlers[static_cast<int>(FPCore::Net::PacketBodyType::PACKET_TYPE_COUNT)];
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

    // Map linking Packet Body Types to their appropriate functions for handling related byte streams.
    FPCore::Net::PacketBodyFuncMap PacketBodyDefFunctionsMap;

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

    void HandleIncomingPacket(FPCore::Net::PacketHead& Packet);
    
    // Writes a packet to be sent to the passed Connection ID into the buffer. Requires successful locking of the
    // internal Write Mutex.
    // Returns whether writing was successful.
    // NOTE: BodyDef is expected to contain an appropriate BodyDataDef structure associated with the Body Type. Depending on the type of body it
    // either contains the entirety of the packet body, or its unmarshalled version with pointers to data that need to be copied aswell.
    bool WriteOutgoingPacket(ServerConnectionID_t DestinationConnectionID, FPCore::Net::PacketBodyType BodyType, void* BodyDefPtr);
    
    // Flushes the internal packed sending buffer to the platform sending buffer, performing appropriate checks and
    // conversions. Returns how many bytes were written.
    size_t FlushSendingBufferToPlatformBuffer(byte* PlatformWriteBuffer, size_t PlatformWriteBufferSize);
};