#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "Windows.h"
#include "WinSock2.h"

#include "iostream"
#include "mutex"

#include "ServerFramework/ServerPlatform.h"

#define MAX_ACTIVE_CONNECTION_COUNT 256

bool bListenThreadRunning = false;
HANDLE ListenThreadHandle = 0;

bool bReceptionThreadRunning = false;
HANDLE ReceptionThreadHandle = 0;

bool bSendingThreadRunning = false;
HANDLE SendingThreadHandle = 0;

struct Win32NetConnection
{
	ServerPlatform::ConnectionID ID;
	
	SOCKET SocketHandle;
	sockaddr_in Address;
	char AddressString[128];
};

std::mutex Mutex_ClientConnectionData;

Win32NetConnection ActiveConnections[MAX_ACTIVE_CONNECTION_COUNT];
WSAEVENT ConnectionEventHandles[MAX_ACTIVE_CONNECTION_COUNT];

// Contains IDs of Sockets that are awaiting Acknowledgement by the Server.
ServerPlatform::ConnectionID PendingConnectionEvents[MAX_ACTIVE_CONNECTION_COUNT];
int PendingConnectionEventsCount = 0;

// Contains IDs of Sockets that have disconnected and need to noticed by the Server and cleaned up. 
ServerPlatform::ConnectionID PendingDisconnectionEvents[MAX_ACTIVE_CONNECTION_COUNT];
int PendingDisconnectionEventsCount = 0;

std::mutex Mutex_NetDataReception;

// #TODO(Marc): Read those from config file ! + Define on Server Platform as Server will also need to know the encoding type for packet size.
#define RECEPTION_BUFFER_SIZE (1024 * 64) // 64kb
static_assert(RECEPTION_BUFFER_SIZE % sizeof(FPCore::Net::PacketBodySize_t) == 0, "STATIC ASSERTION FAILURE: RECEPTION_BUFFER_SIZE must be dividable by PACKET_MAX_SIZE !");

uint8_t ReceptionBuffer[RECEPTION_BUFFER_SIZE];
size_t ReceivedBytes = 0; // Bytes received since last Reception Buffer Clear.

std::mutex Mutex_NetDataSending;
HANDLE Event_DataReadyForSending; // When signaled, the Sending Thread will seek to send out data while locking access to the Sending Buffer in the process.

// #TODO(Marc): Read those from config file !
#define SENDING_BUFFER_SIZE (1024 * 64)

// Contains all data to be sent to active net connections. Sent continuously by the Sending thread.
uint8_t SendingBuffer[SENDING_BUFFER_SIZE];
size_t BytesToSend = 0;

// #TODO(Marc): Should the Reception or even the Connection & Disconnection buffers be Double-buffered instead of locked ?
// I guess it depends on how long the server is going to take to process the data. We don't want to risk losing connections because we take too long to receive data
// in a TCP context.

ServerPlatform::ConnectionID FindAvailableClientIndex()
{
	for (ServerPlatform::ConnectionID ClientIndex = 0; ClientIndex < MAX_ACTIVE_CONNECTION_COUNT; ClientIndex++)
	{
		if (ActiveConnections[ClientIndex].SocketHandle == INVALID_SOCKET)
		{
			return ClientIndex;
		}
	}

	std::cerr << "Error: Maximum number of connections reached!\n";
	return -1;
}

// Disconnects / handles disconnection without clearing the associated data unless it has not been acknowledged
// by the Server yet. Assumes the Client Connection Data Mutex has been appropriately locked.
void Disconnect(ServerPlatform::ConnectionID ConnectionID)
{
	if (ActiveConnections[ConnectionID].SocketHandle == INVALID_SOCKET)
	{
		return;
	}

	WSASendDisconnect(ActiveConnections[ConnectionID].SocketHandle, nullptr);
	
	// Close the associated socket.
	closesocket(ActiveConnections[ConnectionID].SocketHandle);

	// Close associated Event handle.
	WSACloseEvent(ConnectionEventHandles[ConnectionID]);
	
	// Check that disconnected Connection isn't within the Pending Connection Events queue.
	// If it is, then set the handle in the queue as Invalid and immediately clear the Client Data.
	int QueueIndex;
	for(QueueIndex = 0; QueueIndex < PendingConnectionEventsCount; QueueIndex++)
	{
		if (PendingConnectionEvents[QueueIndex] == ActiveConnections[ConnectionID].ID)
		{
			PendingConnectionEvents[QueueIndex] = ServerPlatform::INVALID_ID;
			break;
		}
	}
	
	// If not, Add client to Disconnection queue so that its disconnection can be acknowledged by the Server, at
	// which point its data will be cleared.
	if (QueueIndex == PendingConnectionEventsCount)
	{
		PendingDisconnectionEvents[PendingDisconnectionEventsCount] = ConnectionID;
		PendingDisconnectionEventsCount++;
	}

	// Clear connection data
	ServerPlatform::ConnectionID ClosedSocketHandle = ActiveConnections[ConnectionID].SocketHandle;
	ActiveConnections[ConnectionID].SocketHandle = INVALID_SOCKET;
	ActiveConnections[ConnectionID].Address = {};
	memset(ActiveConnections[ConnectionID].AddressString, 0, sizeof(ActiveConnections[ConnectionID].AddressString));
	
	ConnectionEventHandles[ConnectionID] = 0;

	std::cout << "Socket ID " << ClosedSocketHandle << " closed.\n"; 
}

void HandleNetData(ServerPlatform::ConnectionID ConnectionID, const char* DataPtr, size_t DataSize)
{
	// Check that we have enough memory left in the Reception Buffer for the data itself plus the Connection ID and Data Size indicators.
	size_t TotalRequiredBytes = DataSize + sizeof(FPCore::Net::NetEncodedPacketHead) + sizeof(ServerPlatform::ConnectionID);
	if (TotalRequiredBytes > RECEPTION_BUFFER_SIZE - ReceivedBytes)
	{
		std::cerr << "Out of memory on Win32Net Reception Buffer.\n";
		return;
	}

	// Lock access to Reception Buffer for reminder of the function.
	std::lock_guard<std::mutex> Lock(Mutex_NetDataReception);
	
	// Write received data into reception buffer.
	{
		bool AbortReception = false;

		// Cache the current state of the Reception Buffer in case we need to abort the entire reception midway through.
		size_t PreReadReceivedBytes = ReceivedBytes;
		size_t ReadBytes = 0;
		while(ReadBytes < DataSize)
		{
			const byte* ReadAddress = reinterpret_cast<const byte*>(DataPtr) + ReadBytes;

			// Extract a Net Encoded Packet. If it is valid, build a final Packet from it and add it to the Reception buffer.
			
			const FPCore::Net::NetEncodedPacketHead& EncodedPacket = *reinterpret_cast<const FPCore::Net::NetEncodedPacketHead*>(ReadAddress);
			if (EncodedPacket.BodyType == FPCore::Net::PacketBodyType::INVALID
				|| EncodedPacket.BodyType >= FPCore::Net::PacketBodyType::PACKET_TYPE_COUNT)
			{
				AbortReception = true;
				break;
			}

			FPCore::Net::PacketHead* WritePacketAddress = reinterpret_cast<FPCore::Net::PacketHead*>(ReceptionBuffer + ReceivedBytes);
			WritePacketAddress->ConnectionID = ConnectionID;
			WritePacketAddress->BodyType = EncodedPacket.BodyType;
			WritePacketAddress->BodySize = EncodedPacket.BodySize;

			memcpy(ReceptionBuffer + ReceivedBytes + sizeof(FPCore::Net::PacketHead), ReadAddress + sizeof(FPCore::Net::NetEncodedPacketHead), WritePacketAddress->BodySize);
			WritePacketAddress->BodyStart = ReceptionBuffer + ReceivedBytes + sizeof(FPCore::Net::PacketHead);
			
			ReadBytes += sizeof(FPCore::Net::NetEncodedPacketHead) + EncodedPacket.BodySize;
			ReceivedBytes += sizeof(FPCore::Net::PacketHead) + WritePacketAddress->BodySize;
		}
		
		if (AbortReception)
		{
			// Something didn't line up when decoding the received packets. In this case, abort the entire reception.
			// And close the connection.
			ReceivedBytes = PreReadReceivedBytes;
			std::cerr << "Win32Net Error when receiving packets from Connection ID " << ConnectionID << ". Aborting reception.\n";
			Disconnect(ConnectionID);
		}
	}
}

void HandleNetDisconnection(ServerPlatform::ConnectionID DisconnectedSocketID)
{
	std::cout << "Connection ID " << DisconnectedSocketID << " closed their connection." << std::endl;
	Disconnect(DisconnectedSocketID);
}

// Server listener thread handling new connection requests coming in.
DWORD WINAPI ListenThread_Func(void* Param)
{
	// Create Listen Socket
	SOCKET ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	if (ListenSocket == INVALID_SOCKET)
	{
		std::cerr << "Error creating Listen Socket. Error Code : " << WSAGetLastError() << "\n";
		return 1;
	}

	sockaddr_in ListenSocketAddress = {};
	ListenSocketAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ListenSocketAddress.sin_family = AF_INET;
	ListenSocketAddress.sin_port = htons(25000);

	if (bind(ListenSocket, reinterpret_cast<sockaddr*>(&ListenSocketAddress), sizeof(ListenSocketAddress)) == SOCKET_ERROR)
	{
		std::cerr << "Error binding listen socket. Error Code : " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		return 1;
	}

	if (listen(ListenSocket, 256) == SOCKET_ERROR)
	{
		std::cerr << "Error listening on socket. Error Code : " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		return 1;
	}

	std::cout << "Awaiting Connection...\n";
	bListenThreadRunning = true;
	while (bListenThreadRunning)
	{
		sockaddr_in ConnectedAddr = {};
		int AddressLen = sizeof(ConnectedAddr);
		SOCKET ConnectedSocket = WSAAccept(ListenSocket, reinterpret_cast<sockaddr*>(&ConnectedAddr), &AddressLen, nullptr, 0);

		if (ConnectedSocket == INVALID_SOCKET)
		{
			std::cerr << "Error accepting connection ! Error code = " <<  WSAGetLastError() << "\n";
			continue;
		}

		// Lock access to Client Connection Data for the remainder of the scope.
		std::lock_guard<std::mutex> lock(Mutex_ClientConnectionData);
		
		// Attempt to find an available Client ID.
		ServerPlatform::ConnectionID ConnectionID = FindAvailableClientIndex();

		// Safety checks that could lead to early-refusal of connection.
		if (ConnectionID < 0 // No ID found, likely because capacity was reached. Turn down connection.
			|| PendingConnectionEventsCount >= MAX_ACTIVE_CONNECTION_COUNT) // Platform at capacity on pending connections. Turn down connection.
		{
			std::cerr << "Maximum Client or Pending Connection Event Capacity reached !\n";
			
			WSASendDisconnect(ConnectedSocket, nullptr);
			closesocket(ConnectedSocket);
			continue;
		}

		// Initialize newly connected Client Data
		{
			ActiveConnections[ConnectionID].ID = ConnectionID;
			ActiveConnections[ConnectionID].SocketHandle = ConnectedSocket;
			ActiveConnections[ConnectionID].Address = ConnectedAddr;

			memset(ActiveConnections[ConnectionID].AddressString, 0, sizeof(ActiveConnections[ConnectionID].AddressString));
			DWORD AddressStringLen = sizeof(ActiveConnections[ConnectionID].AddressString);
			WSAAddressToStringA(reinterpret_cast<sockaddr*>(&ConnectedAddr), AddressLen, NULL, ActiveConnections[ConnectionID].AddressString, &AddressStringLen);
		}

		// Initialize newly connected Client Events
		{
			ConnectionEventHandles[ConnectionID] = WSACreateEvent();
			WSAEventSelect(ActiveConnections[ConnectionID].SocketHandle, ConnectionEventHandles[ConnectionID], FD_READ | FD_CLOSE);
		}
		
		// Print newly connected socket handle & address
		std::cout << "New Connection established. [ID " << ConnectionID
		<< " |ADDR " << ActiveConnections[ConnectionID].AddressString
		<< " |HANDLE " << ActiveConnections[ConnectionID].SocketHandle
		<< "]\n";

		// Add Client to Connection Queue for acknowledgement by the server.
		{
			PendingConnectionEvents[PendingConnectionEventsCount] = ConnectionID;
			PendingConnectionEventsCount++;
		}
	}

	bListenThreadRunning = false;
	
	closesocket(ListenSocket);
	return 0;
}

// Server reception thread handling incoming data from existing connections.
DWORD WINAPI ReceptionThread_Func(void* Param)
{
	char PacketReceptionBuffer[1 << 16];

	WSABUF WSAReceptionBuffer;
	WSAReceptionBuffer.buf = PacketReceptionBuffer;
	WSAReceptionBuffer.len = sizeof(PacketReceptionBuffer);

	bReceptionThreadRunning = true;
	
	// Continue running until the Running boolean is internally or externally set to false.
	// Data races on this are not really a concern since only this thread ever sets this to true and only initially.
	while (bReceptionThreadRunning)
	{
		// Continually check for events signaling data is ready to be received.

		WSANETWORKEVENTS NetworkEvents = { 0 };

		// Build an array containing all currently active event handles in sequence and count how many there are.
		WSAEVENT ActiveHandles[MAX_ACTIVE_CONNECTION_COUNT];
		int ActiveHandleCount = 0;

		for (WSAEVENT& ConnectedClientEventHandle : ConnectionEventHandles)
		{
			if (nullptr != ConnectedClientEventHandle)
			{
				ActiveHandles[ActiveHandleCount] = ConnectedClientEventHandle;
				ActiveHandleCount++;
			}
		}
		
		// Wait for up to one second for new events to come in.
		DWORD WaitResult = WSAWaitForMultipleEvents(ActiveHandleCount, ActiveHandles, false, 1000, false);
		
		if (WaitResult >= WSA_WAIT_EVENT_0 && WaitResult < WSA_WAIT_EVENT_0 + MAX_ACTIVE_CONNECTION_COUNT)
		{
			// Event successfuly came in.
			
			// Lock access to Client Connection Data for the remainder of the scope.
			std::lock_guard<std::mutex> lock(Mutex_ClientConnectionData);
			
			// Client ID == Event Handle Index so a direct conversion is good.
			ServerPlatform::ConnectionID ConnectionID = WaitResult - WSA_WAIT_EVENT_0;

			// Determine which event(s) were triggered.
			WSAEnumNetworkEvents(ActiveConnections[ConnectionID].SocketHandle, ConnectionEventHandles[ConnectionID], &NetworkEvents);

			if (NetworkEvents.lNetworkEvents && FD_READ)
			{
				// Receive data into reception buffer and broadcast it.
				DWORD ReceivedBytesCount = 0;
				DWORD Flags = 0;
				if (WSARecv(ActiveConnections[ConnectionID].SocketHandle, &WSAReceptionBuffer, 1, &ReceivedBytesCount, &Flags, NULL, NULL) == 0)
				{
					// It is possible to receive 0 bytes which is a signal for a "Polite goodbye".
					if (ReceivedBytesCount > 0)
					{
						HandleNetData(ConnectionID, WSAReceptionBuffer.buf, ReceivedBytesCount);
					}
					else
					{
						HandleNetDisconnection(ConnectionID);
					}
				}
				else
				{
					// Log the error and close the connection.
					// #TODO(Marc): Some error types are relatively normal and probably shouldn't warrant a log line.
					std::cerr << "Error when receiving data from Connection ID " << ConnectionID << " ! Error Code: " << WSAGetLastError() << std::endl;
					HandleNetDisconnection(ConnectionID);
				}
			}
			else if (NetworkEvents.lNetworkEvents && FD_CLOSE)
			{
				HandleNetDisconnection(ConnectionID);
			}
		}

		// No matter what, simply loop back and start waiting again unless the Reception thread was disabled for some reason.
		memset(PacketReceptionBuffer, 0, sizeof(PacketReceptionBuffer));
	}
	
	bReceptionThreadRunning = false;
	return 0;
}

DWORD WINAPI SendingThread_Func(void* Param)
// Server sending thread handling outgoing data to be sent to existing connections.
{
	// Set up event object which will be used to signal this thread that data is ready to be sent.
	Event_DataReadyForSending = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == Event_DataReadyForSending)
	{
		std::cerr << "Error when creating Data Ready For Sending Event. Error code: " << GetLastError() << "\n";
		bSendingThreadRunning = false;
		return 1;
	}

	bSendingThreadRunning = true;
	while (bSendingThreadRunning)
	{
		// Wait for the event object to be signaled, indicating data is ready to be sent out. When doing so,
		// lock access to the Sending Buffer and send everything out.
		// #TODO(Marc): Double buffer the whole process somehow so throughput isn't limited as much.
		WaitForSingleObject(Event_DataReadyForSending, INFINITE);

		// Lock access to the Sending Buffer until next loop begins.
		Mutex_NetDataSending.lock();

		// The Sending Buffer contains non-encoded packets to be encoded and sent to the relevant connection.
		// Read each packet one by one and send them in separate send calls.
		// #TODO(Marc): Investigate whether there are any performance concerns for sending a lot of small packets to the same connection.

		size_t SendingBufferReadingOffset = 0;
		while(SendingBufferReadingOffset < BytesToSend)
		{
			byte* PacketLocation = SendingBuffer + SendingBufferReadingOffset;
			FPCore::Net::PacketHead OutgoingPacket = *reinterpret_cast<FPCore::Net::PacketHead*>(PacketLocation);
			
			SOCKET OutgoingSocket = ActiveConnections[OutgoingPacket.ConnectionID].SocketHandle;
			if (OutgoingSocket == INVALID_SOCKET)
			{
				// Skip this packet
				SendingBufferReadingOffset += sizeof(FPCore::Net::PacketHead) + OutgoingPacket.BodySize;
				continue;
			}

			char PacketSendingBuffer[1 << 16];
			memset(PacketSendingBuffer, 0, sizeof(PacketSendingBuffer));
			
			FPCore::Net::NetEncodedPacketHead& EncodedOutgoingPacket = *reinterpret_cast<FPCore::Net::NetEncodedPacketHead*>(PacketSendingBuffer);
			EncodedOutgoingPacket.BodyType = OutgoingPacket.BodyType;
			EncodedOutgoingPacket.BodySize = OutgoingPacket.BodySize;

			memcpy_s(PacketSendingBuffer + sizeof(EncodedOutgoingPacket),
				sizeof(PacketSendingBuffer) - sizeof(EncodedOutgoingPacket),
				PacketLocation + sizeof(FPCore::Net::PacketHead), OutgoingPacket.BodySize);
			
			size_t TotalSendSize = sizeof(EncodedOutgoingPacket) + EncodedOutgoingPacket.BodySize;
			send(OutgoingSocket, PacketSendingBuffer, static_cast<int>(TotalSendSize), NULL);

			SendingBufferReadingOffset += sizeof(FPCore::Net::PacketHead) + OutgoingPacket.BodySize;
		}

		// Once all data is sent, set the buffer size back to 0 and zero out its memory.
		memset(SendingBuffer, 0, sizeof(SendingBuffer));
		BytesToSend = 0;

		Mutex_NetDataSending.unlock();
	}
}

bool Win32Net_Init()
{
	// Initialize WSA
	std::cout << "Initializing WinSock Library...\n";

	WSACleanup();
	
	WSAData InitData;
	int Result = WSAStartup(MAKEWORD(2, 2), &InitData);
	if (Result != 0)
	{
		std::cerr << "Error initializing WSA. Error Code : " << WSAGetLastError() << std::endl;
		return false;
	}
	
	// Initialize Client Data
	{
		// Prepare connected client data
		memset(ActiveConnections, 0, sizeof(ActiveConnections));
		for (int ClientIndex = 0; ClientIndex < MAX_ACTIVE_CONNECTION_COUNT; ClientIndex++)
		{
			ActiveConnections[ClientIndex].ID = ClientIndex;
			ActiveConnections[ClientIndex].SocketHandle = INVALID_SOCKET;
		}
	}

	// Create Listen Thread
	{
		std::cout << "Creating Listening Thread.\n";
		ListenThreadHandle = CreateThread(nullptr, NULL, ListenThread_Func, nullptr, NULL, nullptr);

		// Wait for a short while to give time to the thread to initialize and fail on errors.
		Sleep(500);
		if (!bListenThreadRunning)
		{
			std::cerr << "Listen thread failed initialization. Aborting platform net initialization.\n";
			return false;
		}
	}

	// Create Reception Thread
	{
		std::cout << "Creating Reception Thread.\n";
		bReceptionThreadRunning = false;
		ReceptionThreadHandle = CreateThread(NULL, NULL, ReceptionThread_Func, NULL, NULL, NULL);

		// Wait for a short while to give time to the thread to initialize and fail on errors.
		Sleep(500);
		if (!bReceptionThreadRunning)
		{
			std::cerr << "Reception thread failed initialization. Aborting platform net initialization.\n";
			return false;
		}
	}

	// Create Sending Thread
	{
		std::cout << "Creating Sending Thread.\n";
		bSendingThreadRunning = false;
		ReceptionThreadHandle = CreateThread(NULL, NULL, SendingThread_Func, NULL, NULL, NULL);

		// Wait for a short while to give time to the thread to initialize and fail on errors.
		Sleep(500);
		if (!bSendingThreadRunning)
		{
			std::cerr << "Sending thread failed initialization. Aborting platform net initialization.\n";
			return false;
		}
	}
	
	return true;
}

// Returns the pointers to pending connection & disconnection events and specifies their respective counts.
// Locks access to Connected Client Data and Event buffers until ClearNetEvents() is called.
void ReadNetEvents(const ServerPlatform::ConnectionID*& NewConnectionIDs, size_t& OutConnectedCount,
		const ServerPlatform::ConnectionID*& DisconnectedIDs, size_t& OutDisconnectedCount)
{
	// Lock access to Client Connection Data until ClearNetEvents is called.
	Mutex_ClientConnectionData.lock();

	NewConnectionIDs = PendingConnectionEvents;
	OutConnectedCount = PendingConnectionEventsCount;

	DisconnectedIDs = PendingDisconnectionEvents;
	OutDisconnectedCount = PendingDisconnectionEventsCount;
}

// Clears data associated to Connection and Disconnection events. Unlocks access to Connected Client Data.
void ClearNetEvents()
{	
	// Clear Pending Connection & Disconnection event arrays and unlock access to Connected Client Data on the platform.

	memset(PendingConnectionEvents, 0, sizeof(PendingConnectionEvents));
	memset(PendingDisconnectionEvents, 0, sizeof(PendingDisconnectionEvents));

	PendingConnectionEventsCount = 0;
	PendingDisconnectionEventsCount = 0;

	Mutex_ClientConnectionData.unlock();
}

// Returns the pointer to pending Reception Buffer Data and specifies the total amount of received bytes.
// Locks access to the Reception Buffer until ReleaseNetReceptionBuffer() is called.
void ReadNetReceptionBuffer(const byte*& OutReceptionBuffer, size_t& OutReceivedBytesCount)
{
	Mutex_NetDataReception.lock();
	
	OutReceptionBuffer = ReceptionBuffer;
	OutReceivedBytesCount = ReceivedBytes;
}

// Clears the Reception Buffer of all data (by resetting the Received Bytes count to 0) and unlocks access to it.
void ReleaseNetReceptionBuffer()
{
	ReceivedBytes = 0;
	
	Mutex_NetDataReception.unlock();
}

// Returns the pointer to pending Sending Buffer Data and specifies the maximum amount of bytes that can be sent.
// Locks access to the Sending Buffer until ReleaseNetSendingBuffer() is called.
void BeginWritingToSendingBuffer(byte*& OutSendingBuffer, size_t& OutMaxBytes)
{
	Mutex_NetDataSending.lock();
	
	OutSendingBuffer = SendingBuffer + BytesToSend;
	OutMaxBytes = SENDING_BUFFER_SIZE - BytesToSend;
}

// Signal that we are done writing to the Sending Buffer, which will trigger the sending of the data to outgoing
// connections, while releasing access to the Sending Buffer and signaling the Sending Thread to begin work.
void EndWritingToSendingBuffer(size_t SentBytesCount)
{
	// While the Sending Buffer does contain the data, the information of how much is lost.
	// Restore it with the passed Sent Bytes Count parameter.
	BytesToSend += SentBytesCount;
	
	// Unlock access to the Sending Buffer and signal the Sending Thread to begin work.
	Mutex_NetDataSending.unlock();
	SetEvent(Event_DataReadyForSending);
}

// Registers Network-related functions onto the Server platform for use by the Server.
void Win32Net_RegisterPlatformFunctions(ServerPlatform& Platform)
{
	Platform.ReadPlatformNetEvents = ReadNetEvents;
	Platform.ReleasePlatformNetEvents = ClearNetEvents;
	
	Platform.ReadPlatformNetReceptionBuffer = ReadNetReceptionBuffer;
	Platform.ReleasePlatformNetReceptionBuffer = ReleaseNetReceptionBuffer;
	
	Platform.WriteToPlatformNetSendingBuffer = BeginWritingToSendingBuffer;
	Platform.ReleasePlatformNetSendingBuffer = EndWritingToSendingBuffer;

	Platform.CloseConnection = Disconnect;
}

void ShutdownServer()
{
	bListenThreadRunning = false;
	bReceptionThreadRunning = false;

	WaitForSingleObject(ListenThreadHandle, INFINITE);
	WaitForSingleObject(ReceptionThreadHandle, INFINITE);

	CloseHandle(ListenThreadHandle);
	CloseHandle(ReceptionThreadHandle);
}