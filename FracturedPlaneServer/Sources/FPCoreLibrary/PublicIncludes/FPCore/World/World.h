// World.h
// Contains type declarations for common data structures that make up a World,
// as well as textual and program definitions that ensure client and server are aligned.

#pragma once

#include "cstdint"

typedef unsigned char byte;

// The World structure is made up of three levels of varying size two levels of fixed size at the bottom of the subdivision hierarchy.
// World (Unique) -> Cluster (from 1 to any number of islands within interaction distance) -> Island (from a handful to 256 zones) -> Zone (256 x 256 tiles)
// -> Tile (25m x 25m).

// The World contains a Landscape made up of tiles organized in the aforementioned structure, and different kinds of Entities:
// - Parties are mobile entities that contain Characters
// - Sites are stationary entities that "augment" tiles and zones by defining structures, ownership, inhabiting Characters and such.
// - Characters are abstract entities contained in either Sites or Parties, through which most mechanics and player control go.

namespace FPCore
{
    namespace World
    {
        // Indicates the coordinates of a geographical subdivision within its parent subdivision.
        struct Coordinates
        {
            uint16_t X, Y;
        };

        typedef uint64_t ClusterID;
        typedef uint64_t IslandID;
        // Defines the size of one side of a zone in tiles
        constexpr uint16_t ZONE_SIZE_TILES = 100; // 2.5km x 2.5km square
        constexpr uint32_t TILES_PER_ZONE = ZONE_SIZE_TILES * ZONE_SIZE_TILES;
        constexpr uint16_t TILE_SIZE_METERS = 25; // 25m x 25m square

        /*
            Defines the core properties of a Zone as a whole, not including any possible extensions to it such as through the Site system.
            The exact contents of a zone exists in the form of tiles stored separately within the World structure.
            This is used in representation layers - actual data storage format may be different especially on the server.
        */
        struct ZoneDef
        {
            uint16_t MinimumElevation;
            uint16_t MaximumElevation;

            uint16_t AverageTemperature;
            uint16_t AverageRainfall;


            /*  TODO
            *   Climate & Weather
            *   Natural resources
            *   Security level
            */
        };

        /*
            Defines the core properties of a Tile, not including any possible extensions to it such as structures or present entities.
            This is used in representation layers - actual data storage format may be different especially on the server.
        */
        struct TileDef
        {
            uint16_t CenterElevation; // Elevation at the center of the Tile. 
            uint8_t DirtRatio; // 0 = Exposed rock, 256 = Fully covered in pure dirt.
        };

        namespace Entities
        {
            typedef uint32_t CharacterID;
            typedef char CharacterName[32];

            // Once fully constituted this defines the entirety of the data relevant to a Character, given its Site or Party is known.
            struct CharacterDef
            {
                CharacterID ID;
                CharacterName Name;

                /* TODO
                
                    Appearance
                    Personal Inventory
                    Skills
                    Current & Planned Actions

                */
            };

            typedef uint32_t SiteID;
            typedef char SiteName[32];

            // Once fully constituted this defines the entirety of the data relevant to a Site, given its Island is known.
            struct SiteDef
            {
                SiteID ID;
                SiteName Name;

                Coordinates ZoneCoordinates;
                
                CharacterID* PresentCharacters;
                uint16_t PresentCharacterCount;

                /*  TODO:
                
                    Multi-zone sites
                    Ownership
                    Workforce
                    Structures & Resources
                    Specific Character position within site

                */
            };

            typedef uint32_t PartyID;
            typedef char PartyName[32];

            // Once fully constituted this defines the entirety of the data relevant to a Party.
            struct PartyDef
            {
                PartyID ID;
                PartyName Name;
                
                CharacterID* Members;
                uint16_t Size;

                /* TODO
                
                    Party Inventory,
                    Movement / Action data
                */
            };
        }
    }
}