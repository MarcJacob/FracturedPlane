// WorldViewPlayerController.h
// Player Controller used in the World View game state, able to control the camera in an RTS-like fashion, and emit control events to be read by the map level.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "WorldViewPlayerController.generated.h"

UENUM(BlueprintType)
enum class EWorldViewMode : uint8
{
	// View is currently in an invalid state. The game should hold on until an appropriate mode can be determined, perhaps showing a loading screen or black fade.
	Invalid,
	// Island View allows the user to see and interact with a island-level view of the world, being able to look at the zone their character is currently in,
	// neighbouring zones and any other zone they have vision of.
	IslandView,
	// Zone View allows the user to see and interact with the Zone their character is currently in or any other they have vision of in detail on a tile-level.
	ZoneView,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldViewModeChangedDelegate, EWorldViewMode, ViewMode);

/**
 * World View Player Controller is used as Player Controller for the singular World View map which handles viewing the Island map and the Zone map through
 * a seamless zoom in / out system.
 * It does so by handling user inputs as normal, but then mostly broadcasting events that are reacted to by the World View level and various Widgets / Selectables
 * on map.
 * 
 * It expects to control a World View Camera Pawn.
 */
UCLASS(Blueprintable)
class FRACTUREDPLANECLIENTCORE_API AWorldViewPlayerController : public APlayerController
{
	GENERATED_BODY()
public:

	void SetupInputComponent() override;
	void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	void OnPossess(APawn* NewPawn) override;
	void OnUnPossess() override;

	// Input mapping used when in Island View. Exclusive with Zone View mapping.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World View|Input")
	TObjectPtr<UInputMappingContext> IslandViewMappingContext = nullptr;
	
	// Input Mapping used when in Zone View. Exclusive with Island View mapping.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World View|Input")
	TObjectPtr<UInputMappingContext> ZoneViewMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World View|Input")
	TObjectPtr<UInputAction> MovementInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World View|Input")
	TObjectPtr<UInputAction> ZoomInputAction;

	// Current View Mode of the Controller, describing what the user is expecting to see and be able to interact with.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World View")
	EWorldViewMode ViewMode = EWorldViewMode::Invalid;

	// Abstract Zoom Level representing how close or far the view should be positioned.
	// Also used for detecting transitions, where 1.f is the limit for switching to higher level view, and 0.f the limit for switching to lower level view.
	// Upon switching View mode, this will be reset to the halfway point of 0.5.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World View")
	float ZoomLevel = 0.5f;

	// When true, view mode changes will be stalled.
	// Usually set while processing a view mode change animation & load.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World View")
	bool bViewModeChangeLocked = false;

	// General-Purpose delegate called when changing World View Mode for any reason.
	UPROPERTY(BlueprintAssignable)
	FOnWorldViewModeChangedDelegate OnWorldViewModeChanged;

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void OnViewModeChanged_Blueprint(EWorldViewMode PreviousViewMode, EWorldViewMode NewViewMode);

private:

	// Input Action Bindings
	void OnMovementInputAction(const FInputActionInstance& Action);

	void OnZoomInputAction(const FInputActionInstance& Action);

	// Immediately switches to the specified View Mode, calling the appropriate events and resetting zoom level.
	void SwitchToMode(EWorldViewMode NewViewMode);
};
