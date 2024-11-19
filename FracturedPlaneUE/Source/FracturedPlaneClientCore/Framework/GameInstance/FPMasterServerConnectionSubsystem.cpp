#include "FPMasterServerConnectionSubsystem.h"

#include "Sockets.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"

#include "FPCore/Net/WorldSyncPackets.h"

DEFINE_LOG_CATEGORY(FLogFPClientServerConnectionSubsystem);

void UFPMasterServerConnectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	memset(OnPacketReceived, 0, sizeof(OnPacketReceived));
	
	OnPacketReceived[static_cast<int>(FPCore::Net::PacketType::AUTHENTICATION)].BindUObject(this, &UFPMasterServerConnectionSubsystem::HandleAuthenticationResponsePacket);
}

void UFPMasterServerConnectionSubsystem::Deinitialize()
{
	Super::Deinitialize();

	// If connected to the Master Server, close our connection to it.
    if (IsConnected())
    {
    	Disconnect();
    }
}

void UFPMasterServerConnectionSubsystem::Update()
{
	// Communication with the Master Game Server.
	if (IsConnected())
	{
		uint8 ReceptionBuffer[1024 * 64];
		int32 BytesRead = 0;

		if (ConnectionSocket->Recv(ReceptionBuffer, sizeof(ReceptionBuffer), BytesRead))
		{
			if (BytesRead > 0)
			{
				// We received data from the Game Master Server !
				OnDataReceived(ReceptionBuffer, BytesRead);
			}
			else if (ConnectionSocket->GetConnectionState() == SCS_NotConnected)
			{
				// Server gracefully closed connection. This can have a variety of reasons (maintenance mode disallowing
				// normal client connections, server was shut down...)
				OnConnectionLost();
			}
		}
		else // Error on connection.
		{
			OnConnectionLost();
		}
	}
}


// Attempts synchronous connection to the Master Server. If it fails, FailReason will contain a string explaining why,
// on top of the function itself logging a message in the console.
// #TODO(Marc): It would make more sense for the fail reason to be an enum, or to have both in case we want to react
// differently to different Fail Reasons.
bool UFPMasterServerConnectionSubsystem::ConnectToGameMasterServer(FString& FailReason)
{
	if (IsConnected())
	{
		UE_LOG(FLogFPClientServerConnectionSubsystem, Warning, TEXT("Attempted to connect to Master Server when already connected !"));
		return true;
	}

	UE_LOG(FLogFPClientServerConnectionSubsystem, Log, TEXT("Connecting to Master Server..."));
	
	TSharedPtr<FInternetAddr> MasterServerAddress = ISocketSubsystem::Get()->CreateInternetAddr();
	{
		// #TODO(Marc): Read Master Server address from config.
		FIPv4Address MasterServerIPv4Addr;
		if (!FIPv4Address::Parse("127.0.0.1", MasterServerIPv4Addr))
		{
			UE_LOG(FLogFPClientServerConnectionSubsystem, Error, TEXT("Failed to Parse Master Server Address !"));
			FailReason = "Failed to parse Master Server Address !";
			return false;
		}

		MasterServerAddress->SetIp(MasterServerIPv4Addr.Value);
		MasterServerAddress->SetPort(25000);
	}
	
	ConnectionSocket = ISocketSubsystem::Get()->CreateSocket(FName("Stream"),
		"Master Server Connection Socket", FNetworkProtocolTypes::IPv4);

	if (ConnectionSocket == nullptr)
	{
		UE_LOG(FLogFPClientServerConnectionSubsystem, Error, TEXT("Failed to create Master Server connection Socket !"));
		FailReason = "Failed to create Master Server connection Socket !";
		return false;
	}
	
	// Connect to Master Server
	if (!ConnectionSocket->Connect(*MasterServerAddress))
	{
		UE_LOG(FLogFPClientServerConnectionSubsystem, Error, TEXT("Error connecting to Master Server: %s"), ISocketSubsystem::Get()->GetSocketError(ISocketSubsystem::Get()->GetLastErrorCode()));
		AuthenticationInfo.LastFailReason = FText::FromString(TEXT("Failed to connect to Master Server.\n Is it online ?"));
		return false;
	}

	AuthenticationState = EAuthentificationState::CONNECTED;
	BroadcastAuthenticationState();
	
	// Once connected, set the Socket to non-blocking. This allows us to actively wait for data on the main thread.
	// This is fine as it is part of the main engine update loop, so little to no pointless CPU usage.
	ConnectionSocket->SetNonBlocking(true);
	
	UE_LOG(FLogFPClientServerConnectionSubsystem, Log, TEXT("Connected to Game Master Server successfully !"));
	return true;
}

bool UFPMasterServerConnectionSubsystem::SendAuthenticationRequest(FFPClientAuthenticationInfo Info)
{
	AuthenticationInfo = Info;
	if (!IsConnected())
	{
		AuthenticationInfo.LastFailReason = FText::FromString(TEXT("Connection to Master Server lost."));
		return false;
	}
	
	FPCore::Net::PacketData_Authentication AuthRequestPacketData = {};

	char* AuthUsername = TCHAR_TO_ANSI(*AuthenticationInfo.Username);
	strcpy_s(AuthRequestPacketData.Request.Username, sizeof(AuthRequestPacketData.Request.Username), AuthUsername);

	// Encode Packet and send.
	FPCore::Net::Packet AuthRequestPacket(0, FPCore::Net::PacketType::AUTHENTICATION, AuthRequestPacketData);

	FPCore::Net::NetEncodedPacket AuthRequestPacket_Encoded;
	AuthRequestPacket_Encoded.PacketType = AuthRequestPacket.Type;
	AuthRequestPacket_Encoded.DataSize = AuthRequestPacket.DataSize;
	
	byte SendBuffer[sizeof(AuthRequestPacket_Encoded) + sizeof(AuthRequestPacketData)];

	memcpy(SendBuffer, &AuthRequestPacket_Encoded, sizeof(AuthRequestPacket_Encoded));
	memcpy(SendBuffer + sizeof(AuthRequestPacket_Encoded), &AuthRequestPacketData, sizeof(AuthRequestPacketData));
	
	int32 BytesSent;
	if (!ConnectionSocket->Send(SendBuffer, sizeof(SendBuffer), BytesSent))
	{
		AuthenticationInfo.LastFailReason = FText::FromString(TEXT("Connection to Master Server lost."));
		return false;
	}

	AuthenticationState = EAuthentificationState::AUTHENTICATING;
	BroadcastAuthenticationState();
	
	return true;
}

bool UFPMasterServerConnectionSubsystem::IsConnected() const
{
	return ConnectionSocket != nullptr
	&& ConnectionSocket->GetConnectionState() == SCS_Connected;
}

void UFPMasterServerConnectionSubsystem::BroadcastAuthenticationState() const
{
	OnAuthenticationStateChanged.Broadcast(AuthenticationState);
}

void UFPMasterServerConnectionSubsystem::OnDataReceived(uint8* Data, int32 DataSize)
{
	// Decode received packets

	int ReadBytes = 0;
	while(ReadBytes < DataSize)
	{
		FPCore::Net::NetEncodedPacket* EncodedPacket = reinterpret_cast<FPCore::Net::NetEncodedPacket*>(Data + ReadBytes);

		FPCore::Net::Packet Packet = {};
		Packet.Type = EncodedPacket->PacketType;
		Packet.DataSize = EncodedPacket->DataSize;
		Packet.Data = Data + ReadBytes + sizeof(FPCore::Net::NetEncodedPacket);
	
		int PacketTypeIndex = static_cast<int>(Packet.Type);

		if (ensure(PacketTypeIndex > 0 && PacketTypeIndex < static_cast<int>(FPCore::Net::PacketType::PACKET_TYPE_COUNT)))
		{
			OnPacketReceived[PacketTypeIndex].ExecuteIfBound(Packet);
		}

		ReadBytes += sizeof(EncodedPacket) + EncodedPacket->DataSize;
	}
}

void UFPMasterServerConnectionSubsystem::OnConnectionLost()
{
	UE_LOG(FLogFPClientServerConnectionSubsystem, Error, TEXT("Connection to Master Server lost !"))
	
	AuthenticationInfo.LastFailReason= FText::FromString(TEXT("Connection Lost."));
	AuthenticationState = EAuthentificationState::DISCONNECTED;
	
	BroadcastAuthenticationState();
	Disconnect();
}

void UFPMasterServerConnectionSubsystem::Disconnect()
{
	ConnectionSocket->Shutdown(ESocketShutdownMode::ReadWrite);
	ConnectionSocket->Close();
	ConnectionSocket = nullptr;
}

void UFPMasterServerConnectionSubsystem::HandleAuthenticationResponsePacket(FPCore::Net::Packet& Packet)
{
	FPCore::Net::PacketData_Authentication AuthPacketData = {};
	
	// Process Auth Response
	AuthPacketData = Packet.ReadDataAs<FPCore::Net::PacketData_Authentication>();
	if (AuthPacketData.Response.bAccepted)
	{
		UE_LOG(FLogFPClientServerConnectionSubsystem, Log, TEXT("Successfully Authenticated to Master Server !"));
		AuthenticationState = EAuthentificationState::AUTHENTICATED;
		AuthenticationInfo.LastFailReason = FText::FromString(TEXT(""));
	}
	else
	{
		UE_LOG(FLogFPClientServerConnectionSubsystem, Error, TEXT("Failed to Authenticate to Master Server !"));
			
		// #TODO(Marc): Have the server pass the reason why Authentication failed.
		AuthenticationInfo.LastFailReason = FText::FromString(TEXT("Authentication Failed."));
		Disconnect();
	}
	BroadcastAuthenticationState();
}
