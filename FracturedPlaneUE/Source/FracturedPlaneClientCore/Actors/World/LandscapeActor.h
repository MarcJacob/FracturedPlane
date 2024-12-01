// LandscapeActor.h
// Landscape Actors are actors in charge of displaying a specific land mass in World View.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "FPCore/World/World.h"
#include "LandscapeActor.generated.h"

/// Landscape Actor in charge of displaying a specific landmass in world view.
/// Does so by reading the current World Synchronization status and forming a simple dynamic mesh from landscape tile data.
UCLASS(Blueprintable)
class ALandscapeActor : public AActor
{
	GENERATED_BODY()
public:

	ALandscapeActor();
	
	virtual void PostInitializeComponents() override;
	
	void RefreshLandscape(TArray<TArray<bool>> TileTypesGrid);
	
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
