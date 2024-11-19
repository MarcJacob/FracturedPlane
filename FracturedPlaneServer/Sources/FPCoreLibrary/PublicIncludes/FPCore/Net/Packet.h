// FPNetPacket.h
// Defines common Packet Types, Data Structures and buffer-writing and reading functions.

#pragma once

#include "cstdint"
#include "cmath"
#include "string.h"

typedef unsigned char byte;

namespace FPCore
{
    namespace Net
    {
        typedef uint16_t PacketSize_t;  // Type used to encode the size of a single packet. Also defines their max size.
        typedef uint16_t PacketConnectionID_t; // Type used to encode the ID of whatever network communication channel that is relevant to this packet.

        // #TODO(Marc): This should probably use a string hashing system instead.
        enum class PacketType
        {
            INVALID = -1, // Value used to encore invalid packets that had some issue on reception or in their data.
            MESSAGE, // Simple ANSI string message
            AUTHENTICATION, // When received on the Server, is a request. When received on the client, is a response.
            WORLD_SYNC_LANDSCAPE, // Server to Client packet containing data about the landscape of a chunk of the map.
            WORLD_SYNC_ENTITIES,
            PACKET_TYPE_COUNT
        };

        // Describes the connection ID, type, size, and gives eased access to the data of a Packet received from the Network.
        struct Packet
        {
            PacketConnectionID_t ConnectionID;
            PacketType Type;
            PacketSize_t DataSize;
            void* Data;

            // Creates an invalid packet to be built externally.
            Packet() : ConnectionID(0), Type(PacketType::INVALID), DataSize(0), Data(nullptr) {}

            // Creates a full packet, using the passed data to determine data size and data pointer.
            // WARNING: This will create a packet that holds a NON-OWNED pointer to the passed data, meaning it has to have at
            // most the same lifecycle as the data itself.
            template<typename PacketDataType>
            Packet(PacketConnectionID_t ConID, PacketType PacketType, PacketDataType& PacketData) :
            ConnectionID(ConID), Type(PacketType), DataSize(sizeof(PacketDataType)), Data(&PacketData)
            {}
            
            template<typename T>
            const T& ReadDataAs() { return *static_cast<const T*>(Data); }
        };

        // Packet as they are encoded and decoded when sent and received from the network.
        // Normally followed in memory by their associated data of size DataSize.
        struct NetEncodedPacket
        {
            PacketType PacketType;
            PacketSize_t DataSize;
        };
        
        const byte* GetNextPacketFromBuffer(const byte* StartAddress, const byte* EndAddress, Packet& OutPacket);
        
        byte* WritePacketToBuffer(byte* TargetBuffer, PacketConnectionID_t ConnectionID, PacketType Type, const void* Data, PacketSize_t DataSize);
        byte* WritePacketToBuffer(byte* TargetBuffer, const Packet& Packet);
    }
}

// Reads the passed StartAddress as memory containing a Packet structure followed immediately by its associated Data.
// The EndAddress serves as an EXCLUSIVE upper bound beyond which packet data will not be read.
// The returned pointer is equal to EndAddress if no more packets are left, or points to the next location in memory to get a packet
// from, or is null if there was an error (in which case the type of the OutPacket will also be set to Invalid.
inline const byte* FPCore::Net::GetNextPacketFromBuffer(const byte* StartAddress, const byte* EndAddress, Packet& OutPacket)
{
    // Check address validity (At least one minimally sized packet has to fit)
    if (StartAddress >= EndAddress - sizeof(Packet) - 1)
    {
        // StartAddress is out of bounds, or the space between Start and End couldn't possibly fit a packet.
        OutPacket.Type = PacketType::INVALID;
        OutPacket.Data = nullptr;
        OutPacket.DataSize = 0;
        return nullptr;
    }
    
    OutPacket = *reinterpret_cast<const Packet*>(StartAddress);

    // Calculate the address of the next Raw Packet to decode OR end of buffer (in valid cases).
    const byte* NextAddress = StartAddress + sizeof(Packet) + OutPacket.DataSize;
    if (NextAddress > EndAddress)
    {
        // Packet Data is too big for the rest of the buffer. Set the Packet's data pointer to null (but leave the rest
        // alone for debugging) and return nullptr so the reception loop ends.
        OutPacket.Data = nullptr;
        return nullptr;
    }
    
    return NextAddress;
}

// Writes a Packet at the target address, and returns the address right after the newly written packet
// where another packet can be written.
// Note that this is as simple write with no encoding.
// WARNING: This assumes that appropriate size checks have been performed !
inline byte* FPCore::Net::WritePacketToBuffer(byte* TargetBuffer, PacketConnectionID_t ConnectionID, PacketType Type, const void* Data, PacketSize_t DataSize)
{
    Packet& Packet = *reinterpret_cast<FPCore::Net::Packet*>(TargetBuffer);
    Packet.ConnectionID = ConnectionID;
    Packet.Type = Type;
    Packet.DataSize = DataSize;
    
    memcpy(TargetBuffer + sizeof(Packet), Data, DataSize);
    Packet.Data = TargetBuffer + sizeof(Packet);
    return TargetBuffer + sizeof(Packet) + Packet.DataSize;
}

inline byte* FPCore::Net::WritePacketToBuffer(byte* TargetBuffer, const Packet& Packet)
{
    return WritePacketToBuffer(TargetBuffer, Packet.ConnectionID, Packet.Type, Packet.Data, Packet.DataSize);
}


// PACKET DATA TYPES

namespace FPCore
{
    namespace Net
    {
        struct PacketData_Authentication
        {
            union
            {
                struct
                {
                    char Username[32];
                    char Password[32];
                } Request;

                struct
                {
                    bool bAccepted;
                } Response;
            };
        };
    }
}
