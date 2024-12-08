// WorldViewPawn.h
// Pawn used within the World View map.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "WorldViewPawn.generated.h"

/*
	World View Pawn is a very simple Pawn used to control a RTS-Style Camera.
	Will keep its start orientation while allowing full freedom of movement within defined world space bounds, aligned
	to the World Axis. Handles its own movement and thus does NOT have a Movement Component.
*/
UCLASS()
class FRACTUREDPLANECLIENTCORE_API AWorldViewPawn : public APawn
{
	GENERATED_BODY()

public:
	AWorldViewPawn();

	virtual void Tick(float DeltaTime) override;

	// Snaps the Pawn to the normalized Bounds Space position (0,0,0 = Minimum coords | 1,1,1 = Maximum Coords).
	UFUNCTION(BlueprintCallable)
	void SnapToBoundsSpaceNormalizedPosition(FVector BoundsSpaceNormalizedPos);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World View|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	// Minimum World Space bounds this Pawn may exist in.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World View|Camera")
	FVector BoundsMinimumCoordinates = FVector(-50 * 50, -50 * 50, 250);

	// Maximum World Space bounds this Pawn may exist in.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World View|Camera")
	FVector BoundsMaximumCoordinates = FVector(50 * 50, 50 * 50, 1000);
	
	// Movement speed in units per second when at the lowest zoom level.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World View|Camera")
	float BaseMovementSpeed = 1000.f;
};
