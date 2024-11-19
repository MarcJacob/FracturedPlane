// LandscapeActor.h
// Landscape Actors are actors in charge of displaying a specific land mass in World View.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "FPCore/Net/Packet.h"
#include "FPCore/World/World.h"
#include "LandscapeActor.generated.h"

UENUM(BlueprintType)
enum class ELandscapeTileType : uint8
{
	VOID = FPCore::World::TileLandscapeType::Void,
	DIRT = FPCore::World::TileLandscapeType::Dirt,
	TILE_TYPE_COUNT
};

/// Landscape Actor in charge of displaying a specific landmass in world view.
/// Does so by reading the current World Synchronization status and forming a simple dynamic mesh from landscape tile data.
UCLASS(Blueprintable)
class ALandscapeActor : public AActor
{
	GENERATED_BODY()
public:

	ALandscapeActor();
	
	virtual void PostInitializeComponents() override;
	
	void RefreshLandscape(TArray<TArray<ELandscapeTileType>> TileTypesGrid);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TObjectPtr<UProceduralMeshComponent> ProceduralMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TMap<ELandscapeTileType, TObjectPtr<UTexture2D>> TileTypeTexturesMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TMap<ELandscapeTileType, FColor> TileTypeVertexColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landscape Actor")
	TObjectPtr<UMaterial> ProcMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Actor")
	float TileSize = 100.f;
};
