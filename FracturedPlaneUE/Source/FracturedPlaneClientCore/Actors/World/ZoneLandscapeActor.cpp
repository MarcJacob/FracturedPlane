#include "ZoneLandscapeActor.h"

#include "Framework/GameInstance/FPClientGameInstanceBase.h"
#include "Framework/GameInstance/FPMasterServerConnectionSubsystem.h"

AZoneLandscapeActor::AZoneLandscapeActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));

	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh Component"));
	ProceduralMeshComponent->SetupAttachment(RootComponent);
}

void AZoneLandscapeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UFPClientGameInstanceBase* FPClientGameInstance = UFPClientGameInstanceBase::GetFPClientGameInstance(this);
	if (IsValid(FPClientGameInstance))
	{
		// Run a full refresh of the Zone as if the associated World State data had changed.
		OnWorldStateZoneChanged();
	}
	else
	{
		// Run a full refresh of the Zone using Placeholder Data

		// Full Zone (no void)
		TArray<bool> VoidTileFlags;
		VoidTileFlags.SetNum(FPCore::World::TILES_PER_ZONE);
		for (int i = 0; i < FPCore::World::TILES_PER_ZONE; i++)
		{
			VoidTileFlags[i] = true;
		}

		RefreshLandscape(VoidTileFlags);
	}
}

void AZoneLandscapeActor::BeginPlay()
{
	Super::BeginPlay();

	UFPClientGameInstanceBase* FPClientGameInstance = UFPClientGameInstanceBase::GetFPClientGameInstance(this);
	if (ensure(IsValid(FPClientGameInstance)))
	{
		// Subscribe to appropriate Game Instance World State event so Zone Landscape can be kept up to date.
		FPClientGameInstance->GameWorldState->OnWorldStateZoneChange.AddDynamic(this, &AZoneLandscapeActor::OnWorldStateZoneChanged);
	}
}

void AZoneLandscapeActor::OnWorldStateZoneChanged()
{
	UFPClientGameInstanceBase* FPClientGameInstance = UFPClientGameInstanceBase::GetFPClientGameInstance(this);
	RefreshLandscape(FPClientGameInstance->GameWorldState->VoidTileFlagBuffer);
}

void AZoneLandscapeActor::RefreshLandscape(TArray<bool> VoidTileFlags)
{
	if (ProceduralMeshComponent->GetNumSections() > 0)
	{
		ProceduralMeshComponent->ClearAllMeshSections();
	}

	TArray<FVector> Vertex;
	Vertex.Reserve(FPCore::World::TILES_PER_ZONE * 4); // 4 vertex per tile (#TODO(Marc): Do not use duplicate vertex).

	TArray<int> TriangleIndexes;
	TriangleIndexes.Reserve(FPCore::World::TILES_PER_ZONE * 6); // 2 triangles of 3 vertex making up a face

	TArray<FColor> Colors;
	Colors.Reserve(FPCore::World::TILES_PER_ZONE); // One color indication per vertex

	TArray<FVector2D> UV0Coords;
	UV0Coords.Reserve(FPCore::World::TILES_PER_ZONE); // One set of UV0 Coordinates per vertex

	// Fill in Vertex, Triangle and Colors arrays.
	for(int TileIndex = 0; TileIndex < VoidTileFlags.Num(); TileIndex++)
	{
		// Skip void tiles
		if (!VoidTileFlags[TileIndex])
		{
			continue;
		}

		FColor SectionColor = LandscapeVertexColor;
		int TrianglesStartIndex = Vertex.Num();

		int X, Y;
		X = TileIndex / FPCore::World::ZONE_SIZE_TILES;
		Y = TileIndex - FPCore::World::ZONE_SIZE_TILES * X;

		Vertex.Add(FVector(X		* TileSize,	Y		* TileSize, 0.f));
		Vertex.Add(FVector((X+1)	* TileSize,	Y		* TileSize, 0.f));
		Vertex.Add(FVector((X+1)	* TileSize, (Y+1)	* TileSize, 0.f));
		Vertex.Add(FVector(X		* TileSize,	(Y+1)	* TileSize, 0.f));

		Colors.Emplace(SectionColor);
		Colors.Emplace(SectionColor);
		Colors.Emplace(SectionColor);
		Colors.Emplace(SectionColor);

		UV0Coords.Emplace(FVector2D(0, 0));
		UV0Coords.Emplace(FVector2D(1, 0));
		UV0Coords.Emplace(FVector2D(1, 1));
		UV0Coords.Emplace(FVector2D(0, 1));

		TriangleIndexes.Add(TrianglesStartIndex);
		TriangleIndexes.Add(TrianglesStartIndex + 2);
		TriangleIndexes.Add(TrianglesStartIndex + 1);
			
		TriangleIndexes.Add(TrianglesStartIndex);
		TriangleIndexes.Add(TrianglesStartIndex + 3);
		TriangleIndexes.Add(TrianglesStartIndex + 2);
	}
	
	// Create Section for land.
	// TODO: Multiple Sections for land so multiple textures can be supported.

	ProceduralMeshComponent->CreateMeshSection(0, Vertex, TriangleIndexes, 
		TArray<FVector>(), UV0Coords, Colors,
		TArray<FProcMeshTangent>(), false);

	ProceduralMeshComponent->SetMaterial(0, ProcMeshMaterial);

	// Center Mesh Component around Root
	ProceduralMeshComponent->SetRelativeLocation(FVector(-FPCore::World::ZONE_SIZE_TILES * TileSize / 2, -FPCore::World::ZONE_SIZE_TILES * TileSize / 2, 0.f));
}
