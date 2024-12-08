#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FPMasterServerConnectionSubsystem.h"

#include "FPCore/World/World.h"

#include "FPClientGameInstanceBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(FLogFPClientGameInstance, Log, All);

struct Zone
{
	FPCore::World::Coordinates Coordinates;
	FPCore::World::ZoneDef ZoneDef;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldStateZoneChangeDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldStateIslandChangeDelegate);


/*
	Contains the game's current known World State information, including Landscape, entity positions and such. 
	When playing online, this is maintained by data received from the server. When playing solo missions, this is set and updated by simplified
	solo play mechanics.

	This abstracts away the source of World State data - the Game Instance is in charge of plugging whatever source is necessary into it, and receivers can
	read from it directly or subscribe to various events.

	As of now it only supports loading a single Island at a time.
	TODO: Allow loading more islands, probably in a simplified format so their presence can be suggested through their location and shape but without
	superfluous details.
*/
UCLASS(BlueprintType)
class UFPWorldState : public UObject
{
	GENERATED_BODY()
public:

	// General purpose event called when any change has come to the Zone the World State has "focused". Usually happens when the user changes which zone they
	// view or if a major event changed the zone's landscape.
	UPROPERTY(BlueprintAssignable)
	FOnWorldStateZoneChangeDelegate OnWorldStateZoneChange;

	// General purpose event called when any change has come to the Island contained within the World State.
	UPROPERTY(BlueprintAssignable)
	FOnWorldStateIslandChangeDelegate OnWorldStateIslandChange;

	// Bounds of currently loaded island.
	FIntVector2 IslandBoundsSize = FIntVector2::ZeroValue;
	
	// Contains all loaded zones, associated with their Island-space coordinates.
	TArray<Zone> Zones = {};

	// Defines the ID of the currently "viewed" zone, meaning which zone we can currently see at the Tile level. -1 if None.
	int ViewedZoneID = -1;
	
	// Contains the definitions of all tiles within the viewed zone, if any, including void tiles (check VoidTileFlagBuffer to check if a tile is void).
	TArray<FPCore::World::TileDef> ViewedZoneTileDefsBuffer = {};
	
	// For every tile in viewed zone, indicates whether it is void or an actual landscape tile.
	TArray<bool> VoidTileFlagBuffer = {};

	int GetTileIDForCoordinates(int X, int Y) { return X * FPCore::World::ZONE_SIZE_TILES + Y; }

};

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
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFPWorldState> GameWorldState;

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
};