#include "Actors/PlayerControllers/WorldViewPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

void AWorldViewPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Check that core mapping contexts & actions have been assigned.

	check(IsValid(ZoneViewMappingContext));
	check(IsValid(IslandViewMappingContext));

	check(IsValid(ZoomInputAction));
	check(IsValid(MovementInputAction));

	// Link Input Actions
	UEnhancedInputComponent* InputComp = Cast<UEnhancedInputComponent>(InputComponent);

	InputComp->BindAction(MovementInputAction, ETriggerEvent::Triggered, this, &AWorldViewPlayerController::OnMovementInputAction);
	InputComp->BindAction(ZoomInputAction, ETriggerEvent::Triggered, this, &AWorldViewPlayerController::OnZoomInputAction);
}

void AWorldViewPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PostProcessInput(DeltaTime, bGamePaused);

	// Process View Mode change if needed.
	if (!bViewModeChangeLocked)
	{
		if (ViewMode == EWorldViewMode::IslandView && ZoomLevel <= 0.f)
		{
			SwitchToMode(EWorldViewMode::ZoneView);
		}
		else if (ViewMode == EWorldViewMode::ZoneView && ZoomLevel >= 1.f)
		{
			SwitchToMode(EWorldViewMode::IslandView);
		}
	}

	// Transmit Zoom Level as movement input on the Z Axis.
	GetPawn()->AddMovementInput(FVector::UpVector * ZoomLevel);
}

void AWorldViewPlayerController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);

	// Immediately switch to Island View.
	SwitchToMode(EWorldViewMode::IslandView);
}

void AWorldViewPlayerController::OnUnPossess()
{
	// Switch back to Invalid mode.
	SwitchToMode(EWorldViewMode::Invalid);

	Super::OnUnPossess();
}

void AWorldViewPlayerController::OnMovementInputAction(const FInputActionInstance& Action)
{
	FVector VectorVal = Action.GetValue().Get<FVector>();

	GetPawn()->AddMovementInput(VectorVal);
}

void AWorldViewPlayerController::OnZoomInputAction(const FInputActionInstance& Action)
{
	float FloatVal = Action.GetValue().Get<float>();
	ZoomLevel = FMath::Clamp(ZoomLevel - FloatVal, 0.f, 1.f);
}

void AWorldViewPlayerController::SwitchToMode(EWorldViewMode NewViewMode)
{
	// Ensure view mode change isn't locked. If we got here it should warrant a debugger break as this must be checked "upstream".
	if (!ensure(!bViewModeChangeLocked))
	{
		return;
	}

	EWorldViewMode Previous = ViewMode;
	ViewMode = NewViewMode;

	// Switch to appropriate mapping context
	UEnhancedInputLocalPlayerSubsystem* Input = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	
	Input->RemoveMappingContext(IslandViewMappingContext);
	Input->RemoveMappingContext(ZoneViewMappingContext);
	switch (ViewMode)
	{
	case(EWorldViewMode::IslandView):
		Input->AddMappingContext(IslandViewMappingContext, 0);
		break;
	case(EWorldViewMode::ZoneView):
		Input->AddMappingContext(ZoneViewMappingContext, 0);
		break;
	default:
		break;
	}

	OnViewModeChanged_Blueprint(Previous, ViewMode);
	OnWorldViewModeChanged.Broadcast(ViewMode);
}
