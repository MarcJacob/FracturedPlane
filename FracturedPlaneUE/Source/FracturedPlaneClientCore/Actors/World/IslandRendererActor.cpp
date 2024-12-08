#include "Actors/World/IslandRendererActor.h"

#include "Framework/GameInstance/FPClientGameInstanceBase.h"

AIslandRendererActor::AIslandRendererActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));

	ZonesProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Zones Mesh Component"));
	ZonesProceduralMeshComponent->SetupAttachment(RootComponent);
}

FPCore::World::ZoneDef GenerateRandomZoneDef()
{
	using namespace FPCore::World;
	ZoneDef NewZoneDef = {};
	NewZoneDef.MinimumElevation = static_cast<uint16>(FMath::RandRange(0, 600));
	NewZoneDef.MaximumElevation = static_cast<uint16>(FMath::RandRange(NewZoneDef.MinimumElevation, NewZoneDef.MinimumElevation * 3));

	NewZoneDef.AverageRainfall = static_cast<uint8>(FMath::RandRange(0, 255));
	NewZoneDef.AverageTemperature = static_cast<uint8>(FMath::RandRange(0, 255));

	return NewZoneDef;
}

void AIslandRendererActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UFPClientGameInstanceBase* FPClientGameInstance = UFPClientGameInstanceBase::GetFPClientGameInstance(this);
	if (IsValid(FPClientGameInstance) && FPClientGameInstance->GameWorldState->Zones.Num() > 0)
	{
		// Run a full refresh as if the World State Island had just changed.
		OnWorldStateIslandChange();
	}
	else
	{
		// Run a full refresh using Placeholder data.
		TArray<Zone> PlaceholderZones;

		// Generate a random 8 x 8 island with zones placed in a square with random properties.
		PlaceholderZones.Reserve(64);

		for (int ZoneID = 0; ZoneID < 64; ZoneID++)
		{
			int X = ZoneID / 8;
			int Y = ZoneID - (X * 8);

			float VoidChance = 0.05f + (FMath::Abs(X - 4) + FMath::Abs(Y - 4)) / 10.f;
			if (FMath::FRandRange(0.f, 1.f) > VoidChance) 
			{
				// Add zone !
				PlaceholderZones.Emplace(FPCore::World::Coordinates(X, Y), GenerateRandomZoneDef());
			}
		}

		FIntVector2 Bounds = { 8, 8 };

		RefreshZonesMesh(Bounds, PlaceholderZones);
	}
}

void AIslandRendererActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Register to FP Client Game Instance's World State changes.
	UFPClientGameInstanceBase* FPClientGameInstance = UFPClientGameInstanceBase::GetFPClientGameInstance(this);
	if (ensure(IsValid(FPClientGameInstance)))
	{
		FPClientGameInstance->GameWorldState->OnWorldStateIslandChange.AddDynamic(this, &AIslandRendererActor::OnWorldStateIslandChange);
	
		// Assuming that the World State has been updated before this Begin Play function was called, we need to call the event handler now for the first refresh.
		// If the assumption is wrong, the world state will be empty and thus the call will be a negligible waste.
		OnWorldStateIslandChange();
	}
}

void AIslandRendererActor::OnWorldStateIslandChange()
{
	UFPClientGameInstanceBase* FPClientGameInstance = UFPClientGameInstanceBase::GetFPClientGameInstance(this);
	TArray<Zone> IslandZones = FPClientGameInstance->GameWorldState->Zones;

	RefreshZonesMesh(FPClientGameInstance->GameWorldState->IslandBoundsSize, IslandZones);
}

void AIslandRendererActor::RefreshZonesMesh(FIntVector2 BoundsSize, TArray<Zone> Zones)
{
	// Refresh Zones Mesh.

	if (ZonesProceduralMeshComponent->GetNumSections() > 0)
	{
		ZonesProceduralMeshComponent->ClearAllMeshSections();
	}

	TArray<FVector> Vertex;
	Vertex.Reserve(4 * Zones.Num()); // 4 Vertex per zone. TODO: Share vertex when possible ?

	TArray<int> Triangles;
	Triangles.Reserve(6 * Zones.Num()); // 2 Triangles per zone.

	TArray<FColor> Colors;
	Colors.Reserve(4 * Zones.Num()); // Color for each vertex.

	TArray<FVector2D> UV0Coords;

	for (Zone& IslandZone : Zones)
	{
		// Generate color according to average temperature.
		// TODO: Mapmodes, textures, illustrations...

		FColor ZoneColor;
		FLinearColor TemperateZoneColor = FLinearColor(0.131f, 0.085f, 0.035f, 1.f);
		if (IslandZone.ZoneDef.AverageTemperature > 130.f)
		{
			ZoneColor = FMath::Lerp(TemperateZoneColor, FLinearColor::Yellow, (IslandZone.ZoneDef.AverageTemperature - 130.f) / 125.f).ToFColor(true);
		}
		else
		{
			ZoneColor = FMath::Lerp(FLinearColor::Blue, TemperateZoneColor, IslandZone.ZoneDef.AverageTemperature / 130.f).ToFColor(true);
		}

		// Place vertex & assign triangles
		int FirstVertexIndex = Vertex.Num();

		Vertex.Emplace(FVector( ZoneSize * IslandZone.Coordinates.X		, ZoneSize * IslandZone.Coordinates.Y		, 0.f ));
		Vertex.Emplace(FVector(ZoneSize * (IslandZone.Coordinates.X + 1), ZoneSize * IslandZone.Coordinates.Y		, 0.f));
		Vertex.Emplace(FVector(ZoneSize * (IslandZone.Coordinates.X + 1), ZoneSize * (IslandZone.Coordinates.Y + 1)	, 0.f));
		Vertex.Emplace(FVector( ZoneSize * IslandZone.Coordinates.X		, ZoneSize * (IslandZone.Coordinates.Y+1)	, 0.f ));

		Triangles.Emplace(FirstVertexIndex);
		Triangles.Emplace(FirstVertexIndex + 2);
		Triangles.Emplace(FirstVertexIndex + 1);

		Triangles.Emplace(FirstVertexIndex);
		Triangles.Emplace(FirstVertexIndex + 3);
		Triangles.Emplace(FirstVertexIndex + 2);

		Colors.Emplace(ZoneColor);
		Colors.Emplace(ZoneColor);
		Colors.Emplace(ZoneColor);
		Colors.Emplace(ZoneColor);
	}

	ZonesProceduralMeshComponent->CreateMeshSection(0, Vertex, Triangles, TArray<FVector>(), UV0Coords, Colors, TArray<FProcMeshTangent>(), false);
	ZonesProceduralMeshComponent->SetMaterial(0, ZonesMeshMaterial);

	// Center Mesh Component around Root using Island Bounds
	ZonesProceduralMeshComponent->SetRelativeLocation(FVector(-BoundsSize.X * ZoneSize / 2, -BoundsSize.Y * ZoneSize / 2, 0.f));
}