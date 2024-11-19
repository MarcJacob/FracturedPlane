// WorldSyncPackets.h
// Contains common Packet Data Types related to World Synchronization.

#pragma once

#include "FPCore/World/World.h"

namespace FPCore
{
    namespace Net
    {
        // Data linked to a WORLD_SYNC_LANDSCAPE type packet.
        // Contains Landscape data relevant to a specific region of the map delimited by Min and Max Coordinates.
        struct PacketData_LandscapeSync
        {
            World::Coordinates MinCoords;
            World::Coordinates MaxCoords;

            World::Landscape LandscapeData;
        };
    }
}