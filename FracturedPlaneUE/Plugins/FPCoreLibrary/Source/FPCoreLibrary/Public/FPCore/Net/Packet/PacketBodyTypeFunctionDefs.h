// PacketBodyTypeDefs.h
// Contains body type def function definitions + Map initialization
// This is an "Implementation header" that should be included into a single compilation unit somewhere on whatever project makes use of the library.

#pragma once

#include "Packet.h"

// Packet type includes

#include "WorldSyncPackets.h"
#include "AuthenticationPackets.h"

// -- 

// Simplest Get Marshalled Size function, simply returning the size of the underlying Body Def Type, as no extra marshalled data should be present.
template<typename BodyDefType>
size_t GetMarshalledSizeFunc_Simple(void* BodyDef)
{
	return sizeof(BodyDefType);
}

size_t GetMarshalledSizeFunc_ANSIString(void* BodyDef)
{
	return strlen(static_cast<char*>(BodyDef));
}

// Simplest Marshal function - copying the body to the destination as is, given a specific type to work with.
template<typename BodyDefType>
bool MarshalFunc_Simple(void* BodyDef, byte* Dest, size_t DestSize)
{
	return 0 == memcpy_s(Dest, DestSize, BodyDef, GetMarshalledSizeFunc_Simple<BodyDefType>(BodyDef));
}

// Marshal function reinterpreting BodyDef as a null-terminated ANSI string to be copied as-is to Dest.
bool MarshalFunc_ANSIString(void* BodyDef, byte* Dest, size_t DestSize)
{
	return 0 == strcpy_s(reinterpret_cast<char*>(Dest), DestSize, static_cast<char*>(BodyDef));
}

bool MusterFunc_Simple(byte* Body, size_t BodySize)
{
	// Nothing happens - a simple Body def will not have pointers needing to be put in order.
	return true;
}

void FPCore::Net::InitializePacketBodyTypeFunctionsDefMap(PacketBodyFuncMap& Map)
{
	memset(&Map, 0, sizeof(Map));

	Map[PacketBodyType::MESSAGE] =
	{
		GetMarshalledSizeFunc_ANSIString,
		MarshalFunc_ANSIString,
		MusterFunc_Simple
	};

	Map[PacketBodyType::AUTHENTICATION] =
	{
		GetMarshalledSizeFunc_Simple<PacketBodyDef_Authentication>,
		MarshalFunc_Simple<PacketBodyDef_Authentication>,
		MusterFunc_Simple
	};

	Map[PacketBodyType::WORLD_SYNC_LANDSCAPE] =
	{
		GetMarshalledSizeFunc_WorldSyncLandscape,
		MarshalFunc_WorldSyncLandscape,
		MusterFunc_WorldSyncLandscape
	};

	Map[PacketBodyType::WORLD_SYNC_ENTITIES] =
	{

	};
}

// AUTHENTICATION
namespace FPCore
{
    namespace Net
    {
        void MarshalFunc_Authentication(void* BodyDef, void* Dest)
        {
            memcpy(Dest, BodyDef, sizeof(PacketBodyDef_Authentication));
        }
    }
}

// WORLD_SYNC_LANDSCAPE
namespace FPCore
{
    namespace Net
    {
        size_t GetMarshalledSizeFunc_WorldSyncLandscape(void* BodyDef)
        {
            PacketBodyDef_ZoneLandscapeSync& LandscapeSyncBodyDef = *reinterpret_cast<PacketBodyDef_ZoneLandscapeSync*>(BodyDef);
            return sizeof(LandscapeSyncBodyDef); // Body Def
        }

        bool MarshalFunc_WorldSyncLandscape(void* BodyDef, byte* Dest, size_t DestSize)
        {
            PacketBodyDef_ZoneLandscapeSync& LandscapeSyncBodyDef = *reinterpret_cast<PacketBodyDef_ZoneLandscapeSync*>(BodyDef);

            // Check that there is enough room.
            if (GetMarshalledSizeFunc_WorldSyncLandscape(BodyDef) > DestSize)
            {
                return false;
            }

            // Copy the body to dest.
            // More steps will be required when the packet is made to be of variable size.
            memcpy(Dest, BodyDef, sizeof(PacketBodyDef_ZoneLandscapeSync));
            Dest += sizeof(PacketBodyDef_ZoneLandscapeSync);
            size_t TileCount = 256 * 256;

            return true;
        }

        bool MusterFunc_WorldSyncLandscape(byte* Body, size_t BodySize)
        {
            return true;
        }
    }
}
