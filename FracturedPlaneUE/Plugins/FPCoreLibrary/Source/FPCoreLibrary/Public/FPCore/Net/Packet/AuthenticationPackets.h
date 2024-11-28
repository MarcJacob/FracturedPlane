// AuthenticationPacket.h

#pragma once

#include "Packet.h"

namespace FPCore
{
    namespace Net
    {
        struct PacketBodyDef_Authentication
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

        void MarshalFunc_Authentication(void* BodyDef, void* Dest);
    }
}