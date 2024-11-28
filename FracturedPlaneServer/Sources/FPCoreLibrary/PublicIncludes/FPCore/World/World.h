// World.h
// Contains type declarations for common data structures that make up a World.

#pragma once

#include "cstdint"

namespace FPCore
{
    namespace World
    {
        // Indicates the coordinates to a specific tile / area on the world map.
        struct Coordinates
        {
            uint16_t X, Y;
        };
        
        // LANDSCAPE
        enum class TileLandscapeType : uint8_t
        {
            Void,
            Dirt,
            TYPE_COUNT  
         };

        typedef int16_t TileAltitude_t;

        // Contains data for the landscape located within a certain square area.
        // May be used to store the Landscape of an entire Region, or for smaller chunks during synchronization.
        struct Landscape
        {
            int16_t Size; // How large this landscape is. Total tile count will be Size * Size.
            // Per-Tile Buffers.
            TileLandscapeType* TileTypes; // #TODO(Marc): Void tiles should not have associated memory for other tile properties
            // that require land being present.
            TileAltitude_t* TileAltitudes;
        };

        typedef uint32_t GlobalCharacterID;
        typedef char CharacterName[32];
    }
}