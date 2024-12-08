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
	
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Landscape Actor")
	TObjectPtr<UProceduralMeshComponent> ProceduralMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Landscape Actor")
	FColor LandscapeVertexColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Landscape Actor")
	TObjectPtr<UMaterial> ProcMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Landscape Actor")
	float TileSize = 100.f;

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnWorldStateZoneChanged();

	void RefreshLandscape(TArray<bool> VoidTileFlags);
};
