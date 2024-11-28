// WorldSyncPackets.h
// Contains common Packet Data Types related to World Synchronization.

#pragma once

#include "FPCore/World/World.h"
#include "assert.h"

#include "Packet.h"

namespace FPCore
{
    namespace Net
    {
        // Data linked to a WORLD_SYNC_LANDSCAPE type packet.
        // Contains Landscape data relevant to a specific region of the map delimited by Min and Max Coordinates.
        struct PacketBodyDef_LandscapeSync
        {
            // Coordinates of reference for north-western point of synched data, which will form a square.
            World::Coordinates NorthWestCoords;

            World::Landscape LandscapeData;
        };

        size_t GetMarshalledSizeFunc_WorldSyncLandscape(void* BodyDef);
        bool MarshalFunc_WorldSyncLandscape(void* BodyDef, byte* Dest, size_t DestSize);
        bool MusterFunc_WorldSyncLandscape(byte* Body, size_t BodySize);
    }
}