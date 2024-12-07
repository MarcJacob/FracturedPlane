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
        // Data linked to a WORLD_SYNC_ZONE type packet.
        // Contains Landscape data relevant to a specific zone of the map
        // TODO: Variable size in case we don't want to / can't sync the entire zone at once.
        struct PacketBodyDef_ZoneLandscapeSync
        {
            // Coordinates of reference for north-western point of zone. Useful when showing multiple zones.
            World::Coordinates ZoneCoordinates;

            byte VoidTileBitflag[World::TILES_PER_ZONE / 8]; // For each tile, bit is set to 0 if void, 1 if land.
        };

        size_t GetMarshalledSizeFunc_WorldSyncLandscape(void* BodyDef);
        bool MarshalFunc_WorldSyncLandscape(void* BodyDef, byte* Dest, size_t DestSize);
        bool MusterFunc_WorldSyncLandscape(byte* Body, size_t BodySize);
    }
}