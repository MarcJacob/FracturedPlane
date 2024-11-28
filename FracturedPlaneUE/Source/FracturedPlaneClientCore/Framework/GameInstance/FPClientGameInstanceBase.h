#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FPMasterServerConnectionSubsystem.h"

#include "Actors/World/LandscapeActor.h"
#include "FPClientGameInstanceBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(FLogFPClientGameInstance, Log, All);

/**
* Represents the current global state of the game client or server, which determines what to do in certain circumstances.
*/
UENUM(BlueprintType)
enum class EFPGameState : uint8
{
	Setup,			// Game has not properly started yet.
	MainMenu,		// Client is not yet connected and authenticated on the Master Server. 
	Lobby,			// Client is authenticated on the server and has access to account management stuff, character
					// creation and selection...
	Game_WorldView,	// Client is authenticated on the server and is in game.
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChangedDelegate, EFPGameState, GameState);

/**
* Main Game Instance Base class for Fractured Lands Client.
* #TODO(Marc): Move Networking code to a Subsystem for clarity.
*/
UCLASS(Blueprintable)
class FRACTUREDPLANECLIENTCORE_API UFPClientGameInstanceBase : public UGameInstance, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static UFPClientGameInstanceBase* GetFPClientGameInstance(UObject* WorldContextObject);
	
	UFPClientGameInstanceBase(const FObjectInitializer& Init);
	
	virtual void Init() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Shutdown() override;
	
	virtual ETickableTickType GetTickableTickType() const override;
	virtual TStatId GetStatId() const override;

	// Any -> Main Menu transition, mostly use on game start and whenever connection to Master Server is lost.
	// Reason is stored so it can be shown to the Client through the UI system.
	UFUNCTION(BlueprintCallable)
	void GoToMainMenu(FString Reason = "");
	
	// MainMenu -> Pre-game transition, attempting to authenticate to a Master Server. 
	UFUNCTION(BlueprintCallable)
	void LoginToGame(FFPClientAuthenticationInfo AuthenticationInfo);

	// Pre-game -> Game World View transition, beginning the regional sync system with the Master Server and opening up
	// most of the actions available to the client.
	// #TODO(Marc): Entering World View should involve choosing which character to do it as. For now there is one character
	// per account so that is not necessary.
	UFUNCTION(BlueprintCallable)
	void EnterGameWorldView();
	
	UFUNCTION(BlueprintPure)
	EFPGameState GetGameState() const { return GameState;}

	UPROPERTY(BlueprintAssignable)
	FOnGameStateChangedDelegate OnGameStateChanged;

	TArray<TArray<ELandscapeTileType>> TileTypeGrid;
	
private:

	// State of the Game Client. Allows contextual reaction to many events.
	EFPGameState GameState;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu", meta = (AllowPrivateAccess = true))
	FString ReturnToMainMenuReason;
	void OpenMainMenuMap();
	
	void OpenPreGameMap();
	void OpenWorldViewMap();
	
	UFUNCTION()
	void OnMasterServerAuthenticationStateChanged(EAuthentificationState State);
	
	void OnWorldLandscapeSyncPacketReceived(FPCore::Net::PacketHead& Packet);
};