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
        typedef uint16_t PacketBodySize_t;  // Type used to encode the size of a single packet. Also defines their max size.
        typedef uint16_t PacketConnectionID_t; // Type used to encode the ID of whatever network communication channel that is relevant to this packet.

        // #TODO(Marc): This should probably use a string hashing system instead.
        enum PacketBodyType
        {
            INVALID = -1, // Value used to encore invalid packets that had some issue on reception or in their data.
            MESSAGE, // Simple ANSI string message
            AUTHENTICATION, // When received on the Server, is a request. When received on the client, is a response.
            WORLD_SYNC_LANDSCAPE, // Server to Client packet containing data about the landscape of a chunk of the map.
            WORLD_SYNC_ENTITIES,
            PACKET_TYPE_COUNT
        };

        // Contains functions to be associated with a specific packet body type for marshalling, reading and generally interacting with byte streams.
        struct PacketBodyTypeFunctionsDef
        {
            // Returns the total size of the body once marshalled.
            // This performs NO security checks on what is passed !
            size_t (*GetMarshalledSize)(void* BodyDef) = nullptr;
            // Performs the Marshalling operation on the passed body def into the Dest memory, performing a deep copy on data pointed to by the def.
            // Returns whether marshalling was successful.
            bool (*MarshalTo)(void* BodyDef, byte* Dest, size_t DestSize) = nullptr;
            // Performs the Mustering operation on the passed body, making the body def assumed to occupy the front bytes ready for use.
            // Returns whether mustering was successful.
            bool (*Muster)(byte* Body, size_t BodySize) = nullptr;
        };

        typedef PacketBodyTypeFunctionsDef PacketBodyFuncMap[static_cast<size_t>(PacketBodyType::PACKET_TYPE_COUNT)];

        void InitializePacketBodyTypeFunctionsDefMap(PacketBodyFuncMap& Map);

        // Describes the connection ID, type, size, and gives eased access to the data ("body") of a Packet received from the Network.
        struct PacketHead
        {
            PacketConnectionID_t ConnectionID;
            PacketBodyType BodyType;
            PacketBodySize_t BodySize;
            void* BodyStart;

            template<typename T>
            const T& ReadBodyDef() const
            {
                return *reinterpret_cast<T*>(BodyStart);
            }
        };

        // Packet heads as they are encoded and decoded when sent and received from the network.
        // Normally followed in memory by their associated body of size PacketBodySize.
        struct NetEncodedPacketHead
        {
            PacketBodyType BodyType;
            PacketBodySize_t BodySize;
        };
        
        const byte* GetNextPacketFromBuffer(const byte* StartAddress, const byte* EndAddress, PacketHead& OutPacket);
    }
}

// Reads the passed StartAddress as memory containing a Packet structure followed immediately by its associated Data.
// The EndAddress serves as an EXCLUSIVE upper bound beyond which packet data will not be read.
// The returned pointer is equal to EndAddress if no more packets are left, or points to the next location in memory to get a packet
// from, or is null if there was an error (in which case the type of the OutPacket will also be set to Invalid.
inline const byte* FPCore::Net::GetNextPacketFromBuffer(const byte* StartAddress, const byte* EndAddress, PacketHead& OutPacket)
{
    // Check address validity (At least one minimally sized packet has to fit)
    if (StartAddress >= EndAddress - sizeof(PacketHead) - 1)
    {
        // StartAddress is out of bounds, or the space between Start and End couldn't possibly fit a packet.
        OutPacket.BodyType = PacketBodyType::INVALID;
        OutPacket.BodyStart = nullptr;
        OutPacket.BodySize = 0;
        return nullptr;
    }
    
    OutPacket = *reinterpret_cast<const PacketHead*>(StartAddress);

    // Calculate the address of the next Raw Packet to decode OR end of buffer (in valid cases).
    const byte* NextAddress = StartAddress + sizeof(PacketHead) + OutPacket.BodySize;
    if (NextAddress > EndAddress)
    {
        // Packet Data is too big for the rest of the buffer. Set the Packet's body start pointer to null (but leave the rest
        // alone for debugging) and return nullptr so the reception loop ends.
        OutPacket.BodyStart = nullptr;
        return nullptr;
    }
    
    return NextAddress;
}