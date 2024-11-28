#include "ServerFramework/Subsystems/Core/ConnectionsSubsystem.h"

#include <iostream>

#include "ServerFramework/Server.h"

#include "FPCore/Net/Packet/PacketBodyTypeFunctionDefs.h"

ServerConnectionID_t GetAvailableConnectionID(Connection* ConnectionsBuffer, size_t ConnectionsBufferSize)
{
    for(ServerConnectionID_t ServerConnectionID = 0; ServerConnectionID < ConnectionsBufferSize; ServerConnectionID++)
    {
        if (ConnectionsBuffer[ServerConnectionID].PlatformConnectionID == ServerPlatform::INVALID_ID)
        {
            return ServerConnectionID;
        }
    }

    return INVALID_CONNECTION_ID;
}

bool ConnectionsSubsystem::Initialize(MemorySubsystem& Memory, size_t MaxConnection)
{
    // Allocate Connection buffer.
    MaxConnectionCount = MaxConnection;
    ActiveConnections = static_cast<Connection*>(Memory.Allocate(MaxConnectionCount * sizeof(Connection)));
    memset(ActiveConnections, 0, MaxConnectionCount * sizeof(Connection));

    for(ServerConnectionID_t ServerConnectionID = 0; ServerConnectionID < MaxConnectionCount; ServerConnectionID++)
    {
        ActiveConnections[ServerConnectionID].PlatformConnectionID = ServerPlatform::INVALID_ID;
        ActiveConnections[ServerConnectionID].LinkedClient = nullptr;
    }
    
    if (nullptr == ActiveConnections)
    {
        return false;
    }

    PacketWriter.WriteBufferSize = 1024 * 64;
    PacketWriter.WriteBuffer = static_cast<byte*>(Memory.Allocate(PacketWriter.WriteBufferSize));
    PacketWriter.WrittenBytes = 0;

    if (nullptr == PacketWriter.WriteBuffer)
    {
        return false;
    }
    
    FPCore::Net::InitializePacketBodyTypeFunctionsDefMap(PacketBodyDefFunctionsMap);

    return true;
}

void ConnectionsSubsystem::UpdateConnections(float UpdateDeltaTime)
{
    for(ServerConnectionID_t ConnectionID = 0; ConnectionID < MaxConnectionCount; ConnectionID++)
    {
        if (ActiveConnections[ConnectionID].PlatformConnectionID == ServerPlatform::INVALID_ID)
        {
            continue;
        }

        ActiveConnections[ConnectionID].ConnectionUpTime += UpdateDeltaTime;
    }
}

Connection* ConnectionsSubsystem::RegisterConnection(ServerPlatform::ConnectionID ConnectedSocketID)
{
    ServerConnectionID_t AvailableID = GetAvailableConnectionID(ActiveConnections, MaxConnectionCount);
    if (AvailableID == INVALID_CONNECTION_ID)
    {
        return nullptr;
    }

    ActiveConnections[AvailableID].ID = AvailableID;
    ActiveConnections[AvailableID].PlatformConnectionID = ConnectedSocketID;

    ActiveConnections[AvailableID].ConnectionUpTime = 0.f;
    ActiveConnections[AvailableID].LastReceptionTime = 0.f;

    return &ActiveConnections[AvailableID];
}

void ConnectionsSubsystem::DeleteConnection(ServerConnectionID_t ConnectionID)
{
    if (ActiveConnections[ConnectionID].PlatformConnectionID == INVALID_CONNECTION_ID)
    {
        return;
    }
        
    ActiveConnections[ConnectionID].PlatformConnectionID = INVALID_CONNECTION_ID;
    ActiveConnections[ConnectionID].LinkedClient = nullptr;
}

void ConnectionsSubsystem::ConnectClient(ServerConnectionID_t ConnectionID, Client* ClientToConnect)
{
    // Make sure Connection exists, and can be linked to a Client.
    if (ConnectionID > MaxConnectionCount
        || ActiveConnections[ConnectionID].PlatformConnectionID == INVALID_CONNECTION_ID
        || ActiveConnections[ConnectionID].LinkedClient != nullptr)
    {
        return;
    }
    
    ActiveConnections[ConnectionID].LinkedClient = ClientToConnect;
}

Connection* ConnectionsSubsystem::GetConnectionFromPlatformSocket(ServerPlatform::ConnectionID SocketID)
{
    // #TODO(Marc): This is a stupid way of finding the Connection linked to a Socket. Some sort of Map implementation
    // could be helpful.

    for (ServerConnectionID_t ServerConnectionID = 0; ServerConnectionID < MaxConnectionCount; ServerConnectionID++)
    {
        if (ActiveConnections[ServerConnectionID].PlatformConnectionID == SocketID)
        {
            return &ActiveConnections[ServerConnectionID];
        }
    }

    return nullptr;
}

void ConnectionsSubsystem::HandleIncomingPacket(FPCore::Net::PacketHead& Packet)
{
    if (Packet.ConnectionID == ServerPlatform::INVALID_ID
        || Packet.BodyType == FPCore::Net::PacketBodyType::INVALID
        || Packet.BodyType >= FPCore::Net::PacketBodyType::PACKET_TYPE_COUNT)
    {
        return;
    }

    // Check that this message type is handled.
    if (PacketReceptionTable.PacketReceptionHandlers[Packet.BodyType].HandlerFunc == nullptr)
    {
        std::cerr << "Error when handling incoming packet: Packet Body Type " << static_cast<int>(Packet.BodyType)
        << " is not handled !\n";
        return;
    }
    
    // Resolve Socket ID to a Connection ID.
    Connection* InConnection = GetConnectionFromPlatformSocket(Packet.ConnectionID);
    if (nullptr == InConnection)
    {
        std::cerr << "Error when handling incoming packet: Received Packet from Socket ID " << Packet.ConnectionID
        << " which is not bound to a Server Connection !\n";
        return;
    }

    // Change the Packet's Connection ID to actual Connection ID (from being a Platform Socket ID).
    Packet.ConnectionID = InConnection->ID;
    
    // Handle Packet
    // #TODO(Marc): Record time of reception into Connection data.
    PacketReceptionTable.HandlePacket(Packet);
}

#pragma optimize("", off)
bool ConnectionsSubsystem::WriteOutgoingPacket(ServerConnectionID_t DestinationConnectionID, FPCore::Net::PacketBodyType BodyType, void* BodyDefPtr)
{
    // Perform sanity checks
    if (DestinationConnectionID == INVALID_CONNECTION_ID
        || DestinationConnectionID >= MaxConnectionCount
        || ActiveConnections[DestinationConnectionID].PlatformConnectionID == ServerPlatform::INVALID_ID)
    {
        std::cerr << "Error when writing an outgoing packet: Invalid Connection ID !\n";
        return false;
    }

    if (BodyType == FPCore::Net::PacketBodyType::INVALID
        || BodyType >= FPCore::Net::PacketBodyType::PACKET_TYPE_COUNT
        || BodyDefPtr == nullptr)
    {
        std::cerr << "Error when writing an outgoing packet: Invalid Body !\n";
        return false;
    }

    // Dereference body and obtain its size.
    size_t BodySize = PacketBodyDefFunctionsMap[BodyType].GetMarshalledSize(BodyDefPtr);

    // Check that there is enough space within the write buffer.
    size_t RequiredSize = sizeof(FPCore::Net::PacketHead) + BodySize;
    size_t SizeLeft = PacketWriter.WriteBufferSize - PacketWriter.WrittenBytes;
    if (SizeLeft < RequiredSize)
    {
        std::cerr << "Error when writing an outgoing packet: Out of space on the write buffer ! (Required "
        << RequiredSize << ", had " << SizeLeft << ")\n";
        return false;
    }

    byte* WriteLocation = PacketWriter.WriteBuffer + PacketWriter.WrittenBytes;
    byte* WriteStartLoc = WriteLocation;
    
    memset(WriteStartLoc, 0, RequiredSize);

    // Write Packet
    
    FPCore::Net::PacketHead& PacketHead = *reinterpret_cast<FPCore::Net::PacketHead*>(WriteLocation);
    PacketHead.ConnectionID = ActiveConnections[DestinationConnectionID].PlatformConnectionID;
    PacketHead.BodyType = BodyType;
    PacketHead.BodySize = BodySize;

    WriteLocation += sizeof(FPCore::Net::PacketHead);
    SizeLeft -= sizeof(FPCore::Net::PacketHead);

    // Call MarshalTo function associated with this Packet Body Type.
    if (!PacketBodyDefFunctionsMap[BodyType].MarshalTo(BodyDefPtr, WriteLocation, SizeLeft))
    {
        // If Marshalling fails, return immediately, signaling a failure in the packet writing process.
        // Since PacketWriter.WrittenBytes does not get incremented, the next write attempt will happen over the same memory.
        return false;
    }
    PacketHead.BodyStart = WriteLocation;
    WriteLocation += BodySize;

    PacketWriter.WrittenBytes += WriteLocation - WriteStartLoc;

    return true;
}

size_t ConnectionsSubsystem::FlushSendingBufferToPlatformBuffer(byte* PlatformWriteBuffer,
                                                                size_t PlatformWriteBufferSize)
{
    // Empty write buffer into Platform buffer securely (assuming that PlatformWriteBuffer is not null).
    memcpy_s(PlatformWriteBuffer, PlatformWriteBufferSize, PacketWriter.WriteBuffer, PacketWriter.WrittenBytes);
    size_t WrittenBytes = PlatformWriteBufferSize < PacketWriter.WrittenBytes ? PlatformWriteBufferSize : PacketWriter.WrittenBytes;

    // If we couldn't write in the entire Packet Writer buffer this time around, shift the remaining data to the beginning of the
    // buffer so it gets sent in priority next time.
    if (WrittenBytes < PacketWriter.WrittenBytes)
    {
        size_t BytesLeft = PacketWriter.WrittenBytes - WrittenBytes;
        memcpy(PacketWriter.WriteBuffer, PacketWriter.WriteBuffer + PacketWriter.WrittenBytes - BytesLeft, BytesLeft);
        PacketWriter.WrittenBytes = BytesLeft;
    }
    else
    {
        PacketWriter.WrittenBytes = 0;
    }
    
    return WrittenBytes;
}
