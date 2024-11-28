// Platform.h
// Contains abstract Platform-related data and services the Server requires to run.

#pragma once

#include "FPCore/Net/Packet/Packet.h"

// A set of properties and services a Platform has to provide to the Server for it to run appropriately.
// Every function pointer needs to be assigned to something to avoid crashes.
struct ServerPlatform
{
       
    // Data types & typedefs
    
    typedef char* StorePath; // Identifies path to stored data on the platform.
    
    typedef unsigned short ConnectionID; // Identifier identifying a Network Connection on the platform layer. Maximum value indicates invalid value.
    typedef unsigned short ThreadID; // Identifier linking to a Thread on the platform. Maximum value indicates invalid value.
    
    constexpr static unsigned short INVALID_ID = ~0; // Expresses an Invalid value for all Platform Handle types.
    
    // CONTROL & DEBUG
    
    void (*ShutdownProgram)() = nullptr;
    
    // RESOURCES

    // Memory allocated to the server by the platform. It is not possible to allocate more or free it.
    unsigned char* Memory = nullptr;

    // Size of memory allocated to the server by the platform. It is not possible to allocate more or free it.
    size_t MemorySize = 0;
    
    // Creates a new thread and assigns it the passed function.
    ThreadID (*CreateThread)(void(*Func)()) = nullptr;
    // Destroys a thread.
    void (*DestroyThread)(ThreadID& ThreadToDestroy) = nullptr;
    
    // Loads persistent data stored on the platform according to a passed Store Path into the TargetMemory.
    // Data written using a specific Store Path can be retrieved across executions with the exact same Store Path.
    // If the Stored data size is greater than MaxSize, loading will fail.
    // Returns whether loading was successful.
    bool (*LoadStoredData)(StorePath Path, size_t MaxSize, byte* TargetMemory, size_t& LoadedSize) = nullptr;

    // Stores persistent data on the platform according to a passed Store Path.
    // Data written using a specific Store Path can be retrieved across executions with the exact same Store Path.
    bool (*StoreData)(ServerPlatform& Platform, StorePath Path, size_t Size, byte* SourceMemory) = nullptr;

    // Returns pointer to a contiguous sequence of Socket Handles from newly opened connections awaiting Acknowledgement
    // from the server and disconnections since last Read. Out size_t parameters give how many sockets there are in each.
    // As the memory that is pointed to is owned by the platform and not the server, it may get locked.
    // Calling ReleasePlatformNetEvents will unlock it.
    void (*ReadPlatformNetEvents)(const ConnectionID*& OutNewConnectionIDs, size_t& OutConnectedCount,
        const ConnectionID*& OutDisconnectedIDs, size_t& OutDisconnectedCount);

    // Signals the Platform that we are done processing Net Events and that they can be cleared and modified once more.
    void (*ReleasePlatformNetEvents)();

    // Returns pointer to memory containing all data received from the network by the platform since last read.
    // The data has to be organized as a contiguous array of Packets, with the layout being [Packet][Data] for each.
    // As the memory that is pointed to is owned by the platform and not the server, it may get locked.
    // Calling ReleasePlatformNetReceptionBuffer will unlock it.
    void (*ReadPlatformNetReceptionBuffer)(const byte*& OutReceptionBuffer, size_t& OutReceivedBytesCount);

    void (*ReleasePlatformNetReceptionBuffer)();

    // Returns pointer to platform memory where data to be sent to outgoing connections should be put.
    // As the memory that is pointed to is owned by the platform and not the server, it may get locked.
    // Calling ReleasePlatformSendingBuffer will unlock it.
    // Data has to be formatted in the following way for each packet:
    // [ServerPlatform::ID: ConnectionID][PACKET_SIZE_ENCODING_TYPE: PacketSize][DATA].
    void (*WriteToPlatformNetSendingBuffer)(byte*& OutSendingBuffer, size_t& OutBufferSize);
    
    void (*ReleasePlatformNetSendingBuffer)(size_t SentBytesCount);

    // Closes the connection associated with the passed ID.
    void (*CloseConnection)(ConnectionID ConnectionToClose);
};

enum class ShutdownReason
{
    UNKNOWN = 0, // Shutdown reason is unknown / we've reached the Shutdown code without a specific cause.
    PLATFORM_SHUTDOWN = 1, // Platform determined the server should shut down as fast as possible.
    SERVER_SHUTDOWN = 2, // Server requested to be shut down.
};

// Init & Flow functions for the Server to implement, called by Platform.

// Returns how much memory the game server needs at minimum.
size_t GetRequiredServerMemory();
typedef void* GameServerPtr;

// Attempts to create a Game Server framework from the passed Platform data. If successful, returns true and sets the value of the passed
// pointer to the beginning of the memory taken up by the Server Management Data, obfuscated from the Platform layer.
// Otherwise, returns false.
// The MemoryStart pointer inside the Platform data must point to a block of usable memory of appropriate size.
// Use GetRequiredServerMemory() to find out how much it needs at minimum.
bool InitializeServer(const ServerPlatform& Platform, GameServerPtr& OutGameServerPtr);

// Runs an update tick on the server, informing it of the passage of time.
void UpdateServer(GameServerPtr Server, const double& DeltaTime);

// Shuts down the server, giving it a reason.
void ShutdownServer(GameServerPtr Server, const ShutdownReason& Reason);