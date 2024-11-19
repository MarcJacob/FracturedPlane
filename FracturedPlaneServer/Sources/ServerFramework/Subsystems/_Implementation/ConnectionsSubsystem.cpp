#include "ServerFramework/Subsystems/Core/ConnectionsSubsystem.h"

#include <iostream>

#include "ServerFramework/Server.h"

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

    PacketWriter.WriteBuffer = static_cast<byte*>(Memory.Allocate(1024));
    PacketWriter.WriteBufferSize = 1024 * 64;
    PacketWriter.WrittenBytes = 0;

    if (nullptr == PacketWriter.WriteBuffer)
    {
        return false;
    }
    
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

void ConnectionsSubsystem::HandleIncomingPacket(FPCore::Net::Packet& Packet)
{
    if (Packet.ConnectionID == ServerPlatform::INVALID_ID
        || Packet.Type == FPCore::Net::PacketType::INVALID
        || Packet.Type >= FPCore::Net::PacketType::PACKET_TYPE_COUNT)
    {
        return;
    }

    // Check that this message type is handled.
    if (PacketReceptionTable.PacketReceptionHandlers[static_cast<int>(Packet.Type)].HandlerFunc == nullptr)
    {
        std::cerr << "Error when handling incoming packet: Packet Type " << static_cast<int>(Packet.Type)
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
bool ConnectionsSubsystem::WriteOutgoingPacket(const FPCore::Net::Packet& Packet)
{
    // Perform sanity checks
    if (Packet.ConnectionID == INVALID_CONNECTION_ID
        || Packet.ConnectionID >= MaxConnectionCount
        || ActiveConnections[Packet.ConnectionID].PlatformConnectionID == ServerPlatform::INVALID_ID)
    {
        std::cerr << "Error when writing an outgoing packet: Invalid Connection ID !\n";
        return false;
    }

    if (Packet.Type == FPCore::Net::PacketType::INVALID
        || Packet.Type >= FPCore::Net::PacketType::PACKET_TYPE_COUNT
        || Packet.DataSize == 0
        || Packet.Data == nullptr)
    {
        std::cerr << "Error when writing an outgoing packet: Invalid Packet !\n";
        return false;
    }

    // Check that there is enough space within the write buffer.
    size_t RequiredSize = sizeof(FPCore::Net::Packet) + Packet.DataSize;
    size_t SizeLeft = PacketWriter.WriteBufferSize - PacketWriter.WrittenBytes;
    if (SizeLeft < RequiredSize)
    {
        std::cerr << "Error when writing an outgoing packet: Out of space on the write buffer ! (Required "
        << RequiredSize << ", had " << SizeLeft << ")\n";
        return false;
    }
    
    byte* WriteEnd = WritePacketToBuffer(PacketWriter.WriteBuffer + PacketWriter.WrittenBytes, ActiveConnections[Packet.ConnectionID].PlatformConnectionID, Packet.Type, Packet.Data, Packet.DataSize);
    PacketWriter.WrittenBytes = WriteEnd - PacketWriter.WriteBuffer;
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
