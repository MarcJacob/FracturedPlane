// FPMasterServerOnlineSession.h
// Contains declarations linked to main Online Session for the FP Client app, linking it to the Master Server.

#pragma once

#include "CoreMinimal.h"
#include "FPCore/Net/Packet/Packet.h"

#include "FPMasterServerConnectionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(FLogFPClientServerConnectionSubsystem, Log, All);

USTRUCT(BlueprintType)
struct FFPClientAuthenticationInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Username;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Password;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FText LastFailReason;
	// #TODO(Marc): This should be an enum field instead, UI can handle writing proper text from it.
};

UENUM(BlueprintType)
enum class EAuthentificationState : uint8
{
	DISCONNECTED,
	CONNECTED,
	AUTHENTICATING,
	AUTHENTICATED
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthenticationStateChangedDelegate, EAuthentificationState, State);
DECLARE_DELEGATE_OneParam(FOnPacketReceivedDelegate, FPCore::Net::PacketHead&);

UCLASS()
class FRACTUREDPLANECLIENTCORE_API UFPMasterServerConnectionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void Update();

	// Attempts to connect to Master Server. Returns true if successful, false if not. If unsuccessful, FailReason will contain the reason
	// for failure.
	UFUNCTION(BlueprintCallable)
	bool ConnectToGameMasterServer(FString& FailReason);
	
	UFUNCTION(BlueprintCallable)
	bool SendAuthenticationRequest(FFPClientAuthenticationInfo AuthenticationInfo);

	UFUNCTION(BlueprintPure)
	bool IsConnected() const;
	
	// Delegate called when Server accepts Authentication.
	UPROPERTY(BlueprintAssignable)
	FOnAuthenticationStateChangedDelegate OnAuthenticationStateChanged;

	// The last Authentication info that we attempted to use to request Authentication to the Master Server, and
	// potential reason why last authentication failed.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FFPClientAuthenticationInfo AuthenticationInfo;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAuthentificationState AuthenticationState;

	FOnPacketReceivedDelegate OnPacketReceived[static_cast<int>(FPCore::Net::PacketBodyType::PACKET_TYPE_COUNT)];
	
private:
	void BroadcastAuthenticationState() const;

	void Disconnect();
	
	// Main handler function for receiving data from the Master Server.
	void OnDataReceived(uint8* Data, int32 DataSize);

	// Main handler function for receiving a graceful disconnect from the Master Server or when connection was lost
	// for any reason.
	void OnConnectionLost();
	
	FSocket* ConnectionSocket;

	// Packet handlers
	void HandleAuthenticationResponsePacket(FPCore::Net::PacketHead& Packet);

	FPCore::Net::PacketBodyFuncMap PacketBodyTypeFunctionsMap;
};
