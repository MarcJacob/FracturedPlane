#include "Actors/Pawns/WorldViewPawn.h"

AWorldViewPawn::AWorldViewPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComponent->SetupAttachment(RootComponent);

	// Give the Camera Component a default orientation so it looks down at the ground at a slight tilt.
	CameraComponent->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
}

#pragma optimize("", off)
void AWorldViewPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle Movement: Move using Movement Input Vector.
	// X and Y represent desire to move in a specific direction at maximum speed. Z represents the desired zoom level.

	FVector MovementInput = ConsumeMovementInputVector();

	float ZoomLevel = MovementInput.Z;
	MovementInput.Z = 0.f;

	float ZVel = FMath::Lerp(BoundsMinimumCoordinates.Z, BoundsMaximumCoordinates.Z, ZoomLevel) - GetActorLocation().Z;
	
	FVector Velocity = FVector(MovementInput.X * BaseMovementSpeed, MovementInput.Y * BaseMovementSpeed, ZVel * 2);

	FVector NewPosition = GetActorLocation() + Velocity * DeltaTime;

	// Clamp within bounds
	NewPosition.X = FMath::Clamp(NewPosition.X, BoundsMinimumCoordinates.X, BoundsMaximumCoordinates.X);
	NewPosition.Y = FMath::Clamp(NewPosition.Y, BoundsMinimumCoordinates.Y, BoundsMaximumCoordinates.Y);
	NewPosition.Z = FMath::Clamp(NewPosition.Z, BoundsMinimumCoordinates.Z, BoundsMaximumCoordinates.Z);

	SetActorLocation(NewPosition);
}

void AWorldViewPawn::SnapToBoundsSpaceNormalizedPosition(FVector BoundsSpaceNormalizedPos)
{
	FVector SnapPos;
	SnapPos.X = FMath::Lerp(BoundsMinimumCoordinates.X, BoundsMaximumCoordinates.X, BoundsSpaceNormalizedPos.X);
	SnapPos.Y = FMath::Lerp(BoundsMinimumCoordinates.Y, BoundsMaximumCoordinates.Y, BoundsSpaceNormalizedPos.Y);
	SnapPos.Z = FMath::Lerp(BoundsMinimumCoordinates.Z, BoundsMaximumCoordinates.Z, BoundsSpaceNormalizedPos.Z);

	SetActorLocation(SnapPos);
}
