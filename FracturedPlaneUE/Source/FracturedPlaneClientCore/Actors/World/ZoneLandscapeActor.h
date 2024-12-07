// ZoneLandscapeActor.h
// Zone Landscape Actors are actors in charge of displaying a specific zone's landscape in World View.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "FPCore/World/World.h"
#include "ZoneLandscapeActor.generated.h"

/// Zone Landscape Actor in charge of displaying a specific zone's landscape in world view.
/// Does so by reading the current World State and forming a simple dynamic mesh from landscape tile data.
UCLASS(Blueprintable)
class AZoneLandscapeActor : public AActor
{
	GENERATED_BODY()
public:

	AZoneLandscapeActor();
	
	virtual void BeginPlay() override;
	
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void OnWorldStateZoneChanged();

	void RefreshLandscape(TArray<bool> VoidTileFlags);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TObjectPtr<UProceduralMeshComponent> ProceduralMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TObjectPtr<UTexture2D> LandscapeTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	FColor LandscapeVertexColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	FColor VoidVertexColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TObjectPtr<UMaterial> ProcMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Actor")
	float TileSize = 100.f;
};
