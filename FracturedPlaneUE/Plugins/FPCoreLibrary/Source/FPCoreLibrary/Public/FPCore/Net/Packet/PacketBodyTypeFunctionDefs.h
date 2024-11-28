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
            PacketBodyDef_LandscapeSync& LandscapeSyncBodyDef = *reinterpret_cast<PacketBodyDef_LandscapeSync*>(BodyDef);
            size_t TileCount = LandscapeSyncBodyDef.LandscapeData.Size * LandscapeSyncBodyDef.LandscapeData.Size;

            return sizeof(PacketBodyDef_LandscapeSync) // Body Def
                + TileCount * sizeof(LandscapeSyncBodyDef.LandscapeData.TileTypes) // Types total size
                + TileCount * sizeof(LandscapeSyncBodyDef.LandscapeData.TileAltitudes); // Altitudes total size
        }

        bool MarshalFunc_WorldSyncLandscape(void* BodyDef, byte* Dest, size_t DestSize)
        {
            PacketBodyDef_LandscapeSync& LandscapeSyncBodyDef = *reinterpret_cast<PacketBodyDef_LandscapeSync*>(BodyDef);

            // Check that there is enough room.
            if (GetMarshalledSizeFunc_WorldSyncLandscape(BodyDef) > DestSize)
            {
                return false;
            }

            // The data cannot simply be copied to destination as the body def has pointers to unmarshalled data.
            // That data needs to be copied into destination after the body definition.

            memcpy(Dest, BodyDef, sizeof(PacketBodyDef_LandscapeSync));
            Dest += sizeof(PacketBodyDef_LandscapeSync);
            size_t TileCount = LandscapeSyncBodyDef.LandscapeData.Size * LandscapeSyncBodyDef.LandscapeData.Size;

            // Copy Landscape Tile Types
            memcpy(Dest, LandscapeSyncBodyDef.LandscapeData.TileTypes, TileCount * sizeof(FPCore::World::TileLandscapeType));
            Dest += TileCount * sizeof(FPCore::World::TileLandscapeType);
            // Copy Landscape Altitudes
            memcpy(Dest, LandscapeSyncBodyDef.LandscapeData.TileAltitudes, TileCount * sizeof(FPCore::World::TileAltitude_t));
            Dest += TileCount * sizeof(FPCore::World::TileAltitude_t);

            // Erase pointers, to leave no doubt that the data is now Marshalled and must be Mustered to be reconstituted.
            LandscapeSyncBodyDef.LandscapeData.TileTypes = nullptr;
            LandscapeSyncBodyDef.LandscapeData.TileAltitudes = nullptr;

            return true;
        }

        bool MusterFunc_WorldSyncLandscape(byte* Body, size_t BodySize)
        {
            PacketBodyDef_LandscapeSync& LandscapeSyncBodyDef = *reinterpret_cast<PacketBodyDef_LandscapeSync*>(Body);
            Body += sizeof(PacketBodyDef_LandscapeSync);

            // From the body def data, determine the location of internal pointed data from size values.
            size_t TileCount = LandscapeSyncBodyDef.LandscapeData.Size * LandscapeSyncBodyDef.LandscapeData.Size;

            // Muster Tile Type Data
            LandscapeSyncBodyDef.LandscapeData.TileTypes = reinterpret_cast<FPCore::World::TileLandscapeType*>(Body);
            Body += sizeof(FPCore::World::TileLandscapeType) * TileCount;

            // Muster Tile Altitudes
            LandscapeSyncBodyDef.LandscapeData.TileAltitudes = reinterpret_cast<FPCore::World::TileAltitude_t*>(Body);
            Body += sizeof(FPCore::World::TileAltitude_t) * TileCount;

            return true;
        }
    }
}
