#include "LandscapeActor.h"

#include "Framework/GameInstance/FPClientGameInstanceBase.h"
#include "Framework/GameInstance/FPMasterServerConnectionSubsystem.h"

ALandscapeActor::ALandscapeActor()
{
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh Component"));
	ProceduralMeshComponent->SetupAttachment(RootComponent);
}

void ALandscapeActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(GetWorld()->GetGameInstance()))
	{
		RefreshLandscape(UFPClientGameInstanceBase::GetFPClientGameInstance(this)->TileTypeGrid);
	}
}

void ALandscapeActor::RefreshLandscape(TArray<TArray<ELandscapeTileType>> TileTypesGrid)
{
	if (ProceduralMeshComponent->GetNumSections() > 0)
	{
		ProceduralMeshComponent->ClearAllMeshSections();
	}

	const int TileTypeCount = static_cast<int>(ELandscapeTileType::TILE_TYPE_COUNT);
	
	TArray<TArray<FVector>> Vertex;
	Vertex.SetNum(TileTypeCount);
	
	TArray<TArray<int>> TriangleIndexes;
	TriangleIndexes.SetNum(TileTypeCount);
	
	TArray<TArray<FColor>> Colors;
	Colors.SetNum(TileTypeCount);
	
	// Fill in Vertex, Triangle and Colors arrays.
	for(int X = 0; X < TileTypesGrid.Num(); X++)
	{
		Vertex.Reserve(TileTypesGrid[X].Num() * 4); // 4 vertex per tile (#TODO(Marc): Do not use duplicate vertex).
		TriangleIndexes.Reserve(TileTypesGrid[X].Num() * 6); // 2 triangles of 3 vertex making up a face
		Colors.Reserve(TileTypesGrid[X].Num()); // One color indication per vertex
		
		for(int Y = 0; Y < TileTypesGrid[X].Num(); Y++)
		{
			int TileTypeIndex = static_cast<int>(TileTypesGrid[X][Y]);
			
			int TrianglesStartIndex = Vertex[TileTypeIndex].Num();

			Vertex[TileTypeIndex].Add(FVector(X * TileSize,		Y * TileSize, 0.f));
			Vertex[TileTypeIndex].Add(FVector((X+1) * TileSize,	Y * TileSize, 0.f));
			Vertex[TileTypeIndex].Add(FVector((X+1) * TileSize, (Y+1) * TileSize, 0.f));
			Vertex[TileTypeIndex].Add(FVector(X * TileSize,		(Y+1) * TileSize, 0.f));

			Colors[TileTypeIndex].Emplace(TileTypeVertexColorMap[TileTypesGrid[X][Y]]);
			Colors[TileTypeIndex].Emplace(TileTypeVertexColorMap[TileTypesGrid[X][Y]]);
			Colors[TileTypeIndex].Emplace(TileTypeVertexColorMap[TileTypesGrid[X][Y]]);
			Colors[TileTypeIndex].Emplace(TileTypeVertexColorMap[TileTypesGrid[X][Y]]);

			TriangleIndexes[TileTypeIndex].Add(TrianglesStartIndex);
			TriangleIndexes[TileTypeIndex].Add(TrianglesStartIndex + 2);
			TriangleIndexes[TileTypeIndex].Add(TrianglesStartIndex + 1);
			
			TriangleIndexes[TileTypeIndex].Add(TrianglesStartIndex);
			TriangleIndexes[TileTypeIndex].Add(TrianglesStartIndex + 3);
			TriangleIndexes[TileTypeIndex].Add(TrianglesStartIndex + 2);

		}
	}
	
	// Create a section per type of tile
	for(int TileTypeIndex = 0; TileTypeIndex < static_cast<int>(ELandscapeTileType::TILE_TYPE_COUNT); TileTypeIndex++)
	{
		ProceduralMeshComponent->CreateMeshSection(TileTypeIndex, Vertex[TileTypeIndex], TriangleIndexes[TileTypeIndex], TArray<FVector>(), TArray<FVector2D>(), Colors[TileTypeIndex],
			TArray<FProcMeshTangent>(), false);

		ProceduralMeshComponent->SetMaterial(TileTypeIndex, ProcMeshMaterial);
	}
}
