#include "FPClientGameInstanceBase.h"

#include "EngineUtils.h"
#include "Settings/FPGameConfig.h"

#include "FPCore/Net/Packet/WorldSyncPackets.h"

DEFINE_LOG_CATEGORY(FLogFPClientGameInstance);

UFPClientGameInstanceBase* UFPClientGameInstanceBase::GetFPClientGameInstance(UObject* WorldContextObject)
{
	return WorldContextObject->GetWorld()->GetGameInstance<UFPClientGameInstanceBase>();
}

UFPClientGameInstanceBase::UFPClientGameInstanceBase(const FObjectInitializer& Init)
{
	// Create World State Object
	GameWorldState = CreateDefaultSubobject<UFPWorldState>(TEXT("World State"));
}

void UFPClientGameInstanceBase::Init()
{
	Super::Init();

	// Initialize Default Game State to Setup.
	GameState = EFPGameState::Setup;

	UFPMasterServerConnectionSubsystem* MasterServerSubsystem = GetSubsystem<UFPMasterServerConnectionSubsystem>();
	
	// Register Master Server-related events.
	MasterServerSubsystem->OnAuthenticationStateChanged.AddDynamic(this,
	&UFPClientGameInstanceBase::OnMasterServerAuthenticationStateChanged);

	// Assign World State object as target for Master Server Subsystem. TODO: Don't do that right away, in case the user wishes to play solo tutorial missions.
	MasterServerSubsystem->AssignTargetWorldStateObject(GameWorldState);
}

void UFPClientGameInstanceBase::Tick(float DeltaTime)
{
	if (GameState == EFPGameState::Setup)
	{
		return;
	}
	
	// #TODO(Marc): Let's make the entire Subsystem run on a parallel thread so we never have to block the main thread for
	// sending and receiving on the network.
	GetSubsystem<UFPMasterServerConnectionSubsystem>()->Update();
}

void UFPClientGameInstanceBase::Shutdown()
{
	Super::Shutdown();
}

// Specifies that the Game Instance UObject should tick on the main thread in all circumstances.
ETickableTickType UFPClientGameInstanceBase::GetTickableTickType() const
{
	// Game Instance should always tick no matter what while running the game, assuming it's not the class CDO.
	return GetClass()->ClassDefaultObject != this ? ETickableTickType::Always : ETickableTickType::Never;
}

// Makes the Game Instance UObject part of the Main Tickables group for performance statistics collection purposes.
TStatId UFPClientGameInstanceBase::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFPClientGameInstance, STATGROUP_Tickables);
}

void UFPClientGameInstanceBase::GoToMainMenu(FString Reason)
{
	GameState = EFPGameState::MainMenu;
	ReturnToMainMenuReason = Reason;
	OpenMainMenuMap();
}

void UFPClientGameInstanceBase::LoginToGame(FFPClientAuthenticationInfo AuthenticationInfo)
{
	check(GameState == EFPGameState::MainMenu);
	
	if (GetSubsystem<UFPMasterServerConnectionSubsystem>()->SendAuthenticationRequest(AuthenticationInfo))
	{
		// If request was successfully sent, the relevant event handler will make the actual switch to the PreGame State & Map.
	}
}

void UFPClientGameInstanceBase::EnterGameWorldView()
{
	
}

void UFPClientGameInstanceBase::OpenMainMenuMap()
{
	const UFPGameConfig* GameConfig = GetDefault<UFPGameConfig>();
	GEngine->SetClientTravel(WorldContext->World(), *GameConfig->MainMenuMapPath.GetAssetName(), TRAVEL_Absolute);
}

void UFPClientGameInstanceBase::OpenPreGameMap()
{
	const UFPGameConfig* GameConfig = GetDefault<UFPGameConfig>();
	GEngine->SetClientTravel(WorldContext->World(), *GameConfig->PreGameMapPath.GetAssetName(), TRAVEL_Absolute);
}

void UFPClientGameInstanceBase::OpenWorldViewMap()
{
	// Setup Client to travel to World View map.
	
	const UFPGameConfig* GameConfig = GetDefault<UFPGameConfig>();
	GEngine->SetClientTravel(WorldContext->World(), *GameConfig->WorldViewMapPath.GetAssetName(), TRAVEL_Absolute);
}

// When getting authenticated and currently in Main Menu, transition to the Lobby screen.
// When losing authentication, no matter the state, switch back to Main Menu.
void UFPClientGameInstanceBase::OnMasterServerAuthenticationStateChanged(EAuthentificationState State)
{
	if (GameState == EFPGameState::MainMenu && State == EAuthentificationState::AUTHENTICATED)
	{
		// Successfully logged into the game from Main Menu. Switch to Pre Game screen & State.
		GameState = EFPGameState::Lobby;
		OnGameStateChanged.Broadcast(GameState);

		// #TODO(Marc): Character choice feature
		//OpenPreGameMap();
		OpenWorldViewMap();
	}
	else if (GameState != EFPGameState::MainMenu && State == EAuthentificationState::DISCONNECTED)
	{
		// Return to Main Menu immediately.
		// #TODO: Read reason why connection was lost.

		ReturnToMainMenuReason = TEXT("Connection to Master Server lost.");
		ReturnToMainMenu();
	}
}