// IslandRendererActor.h
// Island Renderer Actors are in charge of rendering a single Island landmass, drawing data from World State.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

#include "Framework/GameInstance/FPClientGameInstanceBase.h"

#include "IslandRendererActor.generated.h"

/*
	Island Renderer Actors are in charge of rendering a single Island landmass, drawing data from World State.
	This mostly includes the Island's landscape, but also Zone-wide properties such as climate, fertility and the presence of a Site.
*/
UCLASS(Blueprintable)
class FRACTUREDPLANECLIENTCORE_API AIslandRendererActor : public AActor
{
	GENERATED_BODY()
	
public:	

	AIslandRendererActor();

	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Island Renderer Actor")
	TObjectPtr<UProceduralMeshComponent> ZonesProceduralMeshComponent;

	// Size of the side of a zone square.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Island Renderer Actor")
	float ZoneSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Island Renderer Actor")
	TObjectPtr<UMaterial> ZonesMeshMaterial;

protected:

	virtual void BeginPlay() override;

	void OnWorldStateIslandChange();

	void RefreshZonesMesh(FIntVector2 BoundsSize, TArray<Zone> Zones);
};
