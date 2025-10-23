// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/RoomManager.h"

#include "Components/BoxComponent.h"
#include "DungeonProcedural/Triangle.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Box.h"
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

void URoomManager::GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes)
{
	ClearAll();	
	// Calculate total probability for weighted random selection
	float MaxProba = 0;
	for (FRoomType& RoomType : RoomTypes)
	{
		MaxProba += RoomType.Probability;
	}

	
	// Spawn rooms using weighted random selection based on probability
	for (int i = 0; i < NbRoom; ++i)
	{
		float CurrentProba = FMath::FRandRange(0,MaxProba);
		
		// Find room type using cumulative probability distribution
		float currentMaxProbability = 0;
		FRoomType* RoomType = RoomTypes.FindByPredicate([CurrentProba, &currentMaxProbability](const FRoomType& Room){
			currentMaxProbability += Room.Probability;
			return CurrentProba < currentMaxProbability;
		});

		if (RoomType != nullptr){
			AActor* SpawnedActorRaw = GetWorld()->SpawnActor(RoomType->TypeOfRoomToSpawn);

			// Apply random scaling within defined size range
			FVector RandomScale;
			RandomScale.X = FMath::FRandRange(RoomType->SizeMin, RoomType->SizeMax);
			RandomScale.Y = FMath::FRandRange(RoomType->SizeMin, RoomType->SizeMax);
			RandomScale.Z = 1;
			SpawnedActorRaw->SetActorScale3D(RandomScale);

			
			// Add to spawned actors list if it's a valid room
			if (SpawnedActorRaw->IsA(ARoomParent::StaticClass()))
			{
				SpawnedActors.Add(Cast<ARoomParent>(SpawnedActorRaw));
			}
		}
	}
	// Resolve any overlapping rooms using physics-based separation
	ResolveRoomOverlaps(SpawnedActors);
}

void URoomManager::MegaTriangle(TSubclassOf<ARoomParent> Room)
{
	if (SpawnedActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No actors available to create mega-triangle!"));
		return;
	}
	
	// Initialize bounding box with first room position
	float MaxX = SpawnedActors[0]->GetActorLocation().X;
	float MaxY = SpawnedActors[0]->GetActorLocation().Y;
	float MinX = MaxX;
	float MinY = MaxY;
	
	// Find actual min/max bounds of all rooms
	for (ARoomParent* RoomToUse : SpawnedActors)
	{
		FVector Loc = RoomToUse->GetActorLocation();
		MaxX = FMath::Max(MaxX, Loc.X);
		MinX = FMath::Min(MinX, Loc.X);
		MaxY = FMath::Max(MaxY, Loc.Y);
		MinY = FMath::Min(MinY, Loc.Y);
	}

	float avgX = (MaxX + MinX) / 2;
	float avgY = (MaxY + MinY) / 2;
	float difX = FMath::Abs(MaxX - avgX);
	float difY = FMath::Abs(MaxY - avgY);
	
	
	
	AActor* UpTriangle = GetWorld()->SpawnActor(Room);
	UpTriangle->SetActorLocation(FVector(avgX,avgY+difY*10,0));
	AActor* LeftTriangle = GetWorld()->SpawnActor(Room);
	LeftTriangle->SetActorLocation(FVector(avgX-difX*5,avgY-difY*5,0));
	AActor* RightTriangle = GetWorld()->SpawnActor(Room);
	RightTriangle->SetActorLocation(FVector(avgX+difX*5,avgY-difY*5,0));

	OtherActorsToClear.Add(UpTriangle);
	OtherActorsToClear.Add(LeftTriangle);
	OtherActorsToClear.Add(RightTriangle);
	
	// Store mega-triangle positions for later removal
	MegaTrianglePointA = UpTriangle->GetActorLocation();
	MegaTrianglePointB = LeftTriangle->GetActorLocation();
	MegaTrianglePointC = RightTriangle->GetActorLocation();
	
	AllTriangles.Add(FTriangle(MegaTrianglePointA, MegaTrianglePointB, MegaTrianglePointC));
	FTriangle t = AllTriangles.Last();
	t.DrawTriangle(GetWorld());
}

void URoomManager::Triangulation(TSubclassOf<ARoomParent> RoomP)
{
	TArray<AActor*> RoomPrincipal;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),RoomP,RoomPrincipal);

	if (AllTriangles.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Create mega-triangle first!"));
		return;
	}
	
	// Delaunay triangulation: insert each room point one by one
	for (AActor* Room : RoomPrincipal)
	{
		FVector RoomLocation = Room->GetActorLocation();
		
		// STEP 1: Find all triangles whose circumcircle contains the new point
		TArray<FTriangle> BadTriangles;
		
		// Check ALL existing triangles, not just the recently created ones
		for (const FTriangle& Triangle : AllTriangles)
		{
			FVector CenterCircle;
			if (Triangle.CenterCircle(CenterCircle))
			{
				float Dist = FVector::Dist(CenterCircle, RoomLocation);
				if (Dist <= Triangle.GetRayon())
				{
					BadTriangles.Add(Triangle);
				}
			}
		}
		
		// STEP 2: Remove all invalid triangles
		for (const FTriangle& BadTriangle : BadTriangles)
		{
			AllTriangles.RemoveSingle(BadTriangle);
		}
		
		// STEP 3: Collect all edges from removed triangles
		TArray<FTriangleEdge> AllEdges;
		for (const FTriangle& BadTriangle : BadTriangles)
		{
			TArray<FTriangleEdge> TriangleEdges = BadTriangle.GetEdges();
			AllEdges.Append(TriangleEdges);
		}

		// STEP 4: Identify boundary edges (edges that appear exactly once)
		TArray<FTriangleEdge> BoundaryEdges;
		for (int32 EdgeIdx = 0; EdgeIdx < AllEdges.Num(); ++EdgeIdx)
		{
			if (AllEdges[EdgeIdx].PointA.IsZero() && AllEdges[EdgeIdx].PointB.IsZero()) 
				continue; // Skip edges marked as deleted
			
			int32 Count = 1;
			// Count how many times this edge appears
			for (int32 j = EdgeIdx + 1; j < AllEdges.Num(); ++j)
			{
				if (AllEdges[EdgeIdx] == AllEdges[j])
				{
					Count++;
					// Mark edge as processed
					AllEdges[j].PointA = FVector::ZeroVector;
					AllEdges[j].PointB = FVector::ZeroVector;
				}
			}
			
			// If edge appears exactly once, it's a boundary edge
			if (Count == 1)
			{
				BoundaryEdges.Add(AllEdges[EdgeIdx]);
			}
		}

		// STEP 5: Create new triangles by connecting the point to each boundary edge
		for (const FTriangleEdge& Edge : BoundaryEdges)
		{
			FTriangle NewTriangle(RoomLocation, Edge.PointA, Edge.PointB);
			AllTriangles.Add(NewTriangle);
		}
		
		DrawDebugSphere(GetWorld(), RoomLocation, 50, 50, FColor(0,0,0), true, -1, 0, 50);
	}
	
	// Remove triangles connected to the mega-triangle
	RemoveSuperTriangles();

	TriangulationDone = true;
	
	DrawAll();
	UE_LOG(LogTemp, Display, TEXT("=== [Delaunay Triangles] ==="));
	for (const FTriangle& Tri : AllTriangles)
	{
		UE_LOG(LogTemp, Display, TEXT("Triangle: (%.0f,%.0f) | (%.0f,%.0f) | (%.0f,%.0f)"),
			Tri.PointA.X, Tri.PointA.Y,
			Tri.PointB.X, Tri.PointB.Y,
			Tri.PointC.X, Tri.PointC.Y);
	}
	UE_LOG(LogTemp, Display, TEXT("=============================="));
	UE_LOG(LogTemp, Display, TEXT("Triangulation completed!"))
}

void URoomManager::	CreatePath(TSubclassOf<ARoomParent> RoomP)
{
	if (!TriangulationDone)
	{
		UE_LOG(LogTemp, Warning, TEXT("Complete triangulation first!"));
		return;
	}
	ClearDrawAll();
	TArray<AActor*> RoomPrincipal;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),RoomP,RoomPrincipal);

	if (AllTriangles.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Create mega-triangle first!"));
		return;
	}

	// Prim's algorithm to create minimum spanning tree
	TSet<FVector> PointAlreadyUsed;
	TSet<FTriangleEdge> AllEdges;
	TMap<float,TSet<FTriangleEdge>> PointsSortedByDistance;
	FVector RoomLocation = RoomPrincipal[0]->GetActorLocation();

	// Build MST by selecting shortest edges to unvisited nodes
	while (PointAlreadyUsed.Num() < RoomPrincipal.Num())
	{
		PointAlreadyUsed.Add(RoomLocation);
		
		for (FTriangle TriangleToCheck : AllTriangles)
		{
			if (TriangleToCheck.GetAllPoints().Contains(RoomLocation))
			{
				for (const FTriangleEdge& TrianglesEdge : TriangleToCheck.GetEdges())
				{
					if (PointAlreadyUsed.Contains(TrianglesEdge.PointA) && PointAlreadyUsed.Contains(TrianglesEdge.PointB))
					{
						continue;
					}
					if (TrianglesEdge.PointA == RoomLocation || TrianglesEdge.PointB == RoomLocation)
					{
						AllEdges.Add(TrianglesEdge);
						if (!PointsSortedByDistance.Contains(TrianglesEdge.GetLength()))
						{
							PointsSortedByDistance.Add(TrianglesEdge.GetLength(), TSet<FTriangleEdge>());
						}

						PointsSortedByDistance[TrianglesEdge.GetLength()].Add(TrianglesEdge);
					}
				}
			}
		}

		TArray<float> DistancesToRemove;
		bool HaveToEnd = false;
		
		for (auto& [distance, EdgesSorted] : PointsSortedByDistance)
		{
			TArray<FTriangleEdge> EdgeToRemove;
			for (const FTriangleEdge& EdgeToUse : EdgesSorted)
			{

				if (PointAlreadyUsed.Contains(EdgeToUse.PointA) && PointAlreadyUsed.Contains(EdgeToUse.PointB))
				{
					EdgeToRemove.Add(EdgeToUse);
					continue;
				}
				
				if (EdgeToUse.PointA == RoomLocation)
				{
					RoomLocation = EdgeToUse.PointB;
				}
				else
				{
					RoomLocation = EdgeToUse.PointA;
				}
				
				FirstPath.Add(EdgeToUse);
				EdgeToRemove.Add(EdgeToUse);
				HaveToEnd = true;
				break;
				
			}

			if (EdgeToRemove.Num() > 0)
			{
				for (const FTriangleEdge& ToRemove : EdgeToRemove)
				{
					EdgesSorted.Remove(ToRemove);
					if (EdgesSorted.Num() <= 0)
					{
						DistancesToRemove.Add(distance);
					}
					
				}
			}
			
			if (HaveToEnd)
			{
				break;
			}
			
		}
		
		if (DistancesToRemove.Num() > 0)
		{
			for (const float& DistanceToUse : DistancesToRemove)
			{
				PointsSortedByDistance.Remove(DistanceToUse);
			}
		}
		
	}
	
	UE_LOG(LogTemp, Display, TEXT("=== [MST - FirstPath] ==="));
	for (const FTriangleEdge& Edge : FirstPath)
	{
		UE_LOG(LogTemp, Display, TEXT("Edge: (%.0f,%.0f) -> (%.0f,%.0f)"),
			Edge.PointA.X, Edge.PointA.Y,
			Edge.PointB.X, Edge.PointB.Y);
	}
	UE_LOG(LogTemp, Display, TEXT("==========================="));
	for (const FTriangleEdge& singlePath : FirstPath)
	{
		singlePath.DrawEdge(GetWorld());
	}
	
}

void URoomManager::EvolvePath()
{
	if (!TriangulationDone)
	{
		UE_LOG(LogTemp, Warning, TEXT("Complete triangulation first!"));
		return;
	}
	// Clear previous path visualizations
	ClearDrawAll();
	EvolvedPath.Empty();

	// Transform each MST edge into L-shaped corridors for better navigation
	for (const FTriangleEdge& EdgeToCheck : FirstPath)
	{
		// If rooms are already aligned (horizontal or vertical), keep direct connection
		if (EdgeToCheck.IsStraightLine())
		{
			EvolvedPath.Add(EdgeToCheck);
		}
		else
		{
			// Create L-shaped path for diagonal connections
			FVector Intersection;

			// Two possibilities for L-shape:
			// - (A.x, B.y): horizontal first, then vertical
			// - (B.x, A.y): vertical first, then horizontal
			// Choose randomly for variation
			if (FMath::RandBool())
				Intersection = FVector(EdgeToCheck.PointA.X, EdgeToCheck.PointB.Y, 0);
			else
				Intersection = FVector(EdgeToCheck.PointB.X, EdgeToCheck.PointA.Y, 0);

			// Add the two segments that compose the L-shape
			EvolvedPath.Add(FTriangleEdge(EdgeToCheck.PointA, Intersection));
			EvolvedPath.Add(FTriangleEdge(Intersection, EdgeToCheck.PointB));
		}
	}

	// Draw all generated paths for editor visualization
	for (const auto& Edge : EvolvedPath)
	{
		Edge.DrawEdge(GetWorld());
	}
	UE_LOG(LogTemp, Display, TEXT("=== [Evolved Path - L-shapes] ==="));
	for (const FTriangleEdge& Edge : EvolvedPath)
	{
		UE_LOG(LogTemp, Display, TEXT("Path: (%.0f,%.0f) -> (%.0f,%.0f)"),
			Edge.PointA.X, Edge.PointA.Y,
			Edge.PointB.X, Edge.PointB.Y);
	}
	UE_LOG(LogTemp, Display, TEXT("==============================="));

}

void URoomManager::ClearSecondaryRoom(TSubclassOf<ARoomParent> SecondaryRoomType)
{
	if (!TriangulationDone)
	{
		UE_LOG(LogTemp, Warning, TEXT("Complete triangulation first!"));
		return;
	}
	TArray<AActor*> AllSecondaryRoom;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),SecondaryRoomType,AllSecondaryRoom);

	if (AllTriangles.Num() <= 0)
	{
		return;
	}

	// Check if evolved path exists
	if (EvolvedPath.Num() == 0)
	{
		return;
	}

	// Remove secondary rooms that don't intersect with corridor paths
	for (int SecondaryRoomIndex = 0; SecondaryRoomIndex < AllSecondaryRoom.Num(); ++SecondaryRoomIndex)
	{
		AActor* SecondaryRoom = AllSecondaryRoom[SecondaryRoomIndex];
		if (!SecondaryRoom) continue;
		
		ARoomParent* RoomParent = Cast<ARoomParent>(SecondaryRoom);
		if (!RoomParent || !RoomParent->BoxCollision) 
		{
			continue;
		}
		
		// Use actual BoxCollision size for accurate intersection testing
		FVector BoxExtent = RoomParent->BoxCollision->GetScaledBoxExtent();
		FVector RoomLocation = SecondaryRoom->GetActorLocation();
		
		bool IsInPath = false;
		
		// Check if room intersects with at least one corridor segment
		for (const FTriangleEdge& EdgeToCheck : EvolvedPath)
		{
			// Use BoxExtent * 2 to get full box size
			if (IsSegmentIntersectingBox(EdgeToCheck.PointA, EdgeToCheck.PointB, RoomLocation, BoxExtent * 2.0f))
			{
				IsInPath = true;
				break;
			}
		}
		
		// Destroy room only if it's NOT in the path
		if (!IsInPath)
		{
			SecondaryRoom->Destroy();
		}

	}

}
// Ray-box intersection test for corridor-room collision detection
// Width = X, Height = Z, Depth = Y
bool URoomManager::IsSegmentIntersectingBox(const FVector& PointA, const FVector& PointB, const FVector& CenterOfBox, FVector Size)
{
	FVector Extent = Size* 0.5;
	FBox Box(CenterOfBox - Extent, CenterOfBox + Extent);

	// Check if either endpoint is inside the box
	if (Box.IsInside(PointA) || Box.IsInside(PointB))
		return true;

	float tmin = 0.0f;
	float tmax = 1.0f;

	FVector d = PointB - PointA;

	for (int i = 0; i < 3; i++)
	{
		if (FMath::Abs(d[i]) < KINDA_SMALL_NUMBER)
		{
			// Segment parallel to box faces
			if (PointA[i] < Box.Min[i] || PointA[i] > Box.Max[i])
				return false;
		}
		else
		{
			float ood = 1.0f / d[i];
			float t1 = (Box.Min[i] - PointA[i]) * ood;
			float t2 = (Box.Max[i] - PointA[i]) * ood;

			if (t1 > t2) Swap(t1, t2);

			tmin = FMath::Max(tmin, t1);
			tmax = FMath::Min(tmax, t2);

			if (tmin > tmax)
				return false;
		}
	}

	return true;
}

// Main overlap resolution function using physics-based separation
void URoomManager::ResolveRoomOverlaps(TArray<ARoomParent*>& SpawnedActorsRaw)
{
    if (SpawnedActorsRaw.Num() == 0) return;

	// Randomize processing order for more varied results
	TArray<ARoomParent*> SpawnedActorsToUse = SpawnedActorsRaw;

	int32 Num = SpawnedActorsToUse.Num();
	for (int32 i = 0; i < Num - 1; i++)
	{
		int32 SwapIndex = FMath::RandRange(i, Num - 1);
		if (i != SwapIndex)
		{
			SpawnedActorsToUse.Swap(i, SwapIndex);
		}
	}
    
    for (ARoomParent* CurrentRoom : SpawnedActorsToUse)
    {
        if (!CurrentRoom || !CurrentRoom->BoxCollision) continue;

        FVector LocationA = CurrentRoom->GetActorLocation();
        FVector RepulsionDirection = FVector(
			FMath::FRandRange(-1, 1.), 
			FMath::FRandRange(-1, 1.),
			0
		);
    	
    	
        bool haveToMove = true;
        while (haveToMove)
        {
			haveToMove = false;

			for (ARoomParent*& RoomToCheck : SpawnedActorsToUse)
			{
				if (!RoomToCheck || !RoomToCheck->BoxCollision) continue;
				if (RoomToCheck == CurrentRoom) continue;

				if (CheckOverlapping(CurrentRoom->BoxCollision, RoomToCheck->BoxCollision))
				{
					haveToMove = true;
					if (!RepulsionDirection.IsNearlyZero())
					{
						RepulsionDirection.Normalize();
		            
						float MoveDistance = 1000.0f;
						CurrentRoom->AddActorLocalOffset(RepulsionDirection * MoveDistance);
						break;
					}
				}
			}
        	
        }

    }
}


bool URoomManager::CheckOverlapping(const UBoxComponent* BoxA, const UBoxComponent* BoxB)
{
	if (!BoxA || !BoxB) return false;

	FVector CenterA = BoxA->GetComponentLocation();
	FVector ExtentA = BoxA->GetScaledBoxExtent();

	FVector CenterB = BoxB->GetComponentLocation();
	FVector ExtentB = BoxB->GetScaledBoxExtent();

	return (FMath::Abs(CenterA.X - CenterB.X) <= (ExtentA.X + ExtentB.X)) &&
		   (FMath::Abs(CenterA.Y - CenterB.Y) <= (ExtentA.Y + ExtentB.Y)) &&
		   (FMath::Abs(CenterA.Z - CenterB.Z) <= (ExtentA.Z + ExtentB.Z));
}

void URoomManager::AutoTick()
{
	if (!bAutoDemo)
		return;

	// Advance one step in the demo (reuses manual logic)
	StepByStep(AutoRoomP, AutoRoomS, AutoRoomC);

	// Stop condition: all major steps are finished
	// (0:Triang, 1:Prim, 2:L, 3:Clear, 4:Corridors)
	if (CurrentStep > 4)
	{
		StopAutoDemo();
		UE_LOG(LogTemp, Display, TEXT("Auto-demo completed all steps."));
	}

	// Protection: if someone calls ClearAll in the middle, stop cleanly
	// (you can keep or remove this, it's just a safety measure)
	if (AllTriangles.Num() == 0 && CurrentStep == 0)
	{
		StopAutoDemo();
		UE_LOG(LogTemp, Warning, TEXT("Aborted (no triangles). Did you ClearAll?"));
	}
}

void URoomManager::StopAutoDemo()
{
	bAutoDemo = false;
	GetWorld()->GetTimerManager().ClearTimer(AutoDemoTimer);
	UE_LOG(LogTemp, Display, TEXT("STOP Auto Play"));
}


void URoomManager::DrawAll()
{
	ClearDrawAll();
	if (AllTriangles.Num() > 0)
	{
		for (FTriangle& Triangle : AllTriangles)
		{
			Triangle.DrawTriangle(GetWorld());
		}
	}
}

void URoomManager::ClearDrawAll()
{
	FlushPersistentDebugLines(GetWorld());
}

void URoomManager::ClearAll()
{
	AllTriangles.Empty();
	TriangleErased.Empty();
	LastTrianglesCreated.Empty();
	FirstPath.Empty();
	EvolvedPath.Empty();
	ResetStepByStep();
	RemoveSuperTriangles();
	
	TriangulationDone = false;
	
	for (ARoomParent* SpawnedActor : SpawnedActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}
	SpawnedActors.Empty();

	for (AActor* SpawnedActor : OtherActorsToClear)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}
	OtherActorsToClear.Empty();
	ClearDrawAll();

	CurrentStep = 0;
	CurrentLittleStep = 0;
}

void URoomManager::RemoveSuperTriangles()
{
	// Remove all triangles that share a vertex with the mega-triangle
	AllTriangles.RemoveAll([this](const FTriangle& Triangle) {
		TArray<FVector> Points = Triangle.GetAllPoints();
		for (const FVector& Point : Points)
		{
			if (Point.Equals(MegaTrianglePointA, 1.0f) || 
				Point.Equals(MegaTrianglePointB, 1.0f) || 
				Point.Equals(MegaTrianglePointC, 1.0f))
			{
				return true; // Remove this triangle
			}
		}
		return false; // Keep this triangle
	});
}

template<typename T>
const T* URoomManager::GetAnyElement(const TSet<T>& Set)
{
	if (Set.Num() == 0)
		return nullptr;

	auto It = Set.CreateConstIterator();
	return &(*It);
}

void URoomManager::SpawnConnectionModules(TSubclassOf<AActor> CorridorBP)
{
	if (!TriangulationDone)
	{
		UE_LOG(LogTemp, Warning, TEXT("Complete triangulation first!"));
		return;
	}
	ClearDrawAll();
	if (EvolvedPath.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No evolved paths available to create connection modules."));
		return;
	}

	for (const FTriangleEdge& Edge : EvolvedPath)
	{
		FVector Start = Edge.PointA;
		FVector End = Edge.PointB;

		FVector Direction = End - Start;
		float Length = Direction.Length();
		Direction.Normalize();

		// Calculate median position to place corridor
		FVector Middle = Start + (Direction * (Length / 2));

		// Calculate rotation
		FRotator Rotation = Direction.Rotation();

		// Spawn corridor
		FActorSpawnParameters SpawnParams;
		AActor* Corridor = GetWorld()->SpawnActor<AActor>(CorridorBP, Middle, Rotation, SpawnParams);
		if (!Corridor) continue;

		// Scale according to segment length
		FVector Scale = Corridor->GetActorScale3D();
		Scale.X = Length / 100.0f; // Adjust "100" based on base mesh length
		Corridor->SetActorScale3D(Scale);

		// Optional: keep reference for cleanup later
		OtherActorsToClear.Add(Corridor);
	}

	UE_LOG(LogTemp, Display, TEXT("✅ Connection modules generated (%d segments)."), EvolvedPath.Num());
}

void URoomManager::StepByStep(TSubclassOf<ARoomParent> RoomP,TSubclassOf<ARoomParent> RoomS,TSubclassOf<ARoomParent> RoomC)
{
	UE_LOG(LogTemp, Display, TEXT("=== STEP BY STEP === Step: %d | SubStep: %d"), CurrentStep, CurrentLittleStep);

	switch (CurrentStep)
	{
	case 0: StepByStepTriangulation(RoomP); break;
	case 1: StepByStepPrim(RoomP); break;
	case 2: StepByStepEvolvePath(); break;
	case 3: StepByStepClear(RoomS); break;
	case 4: StepByStepCorridors(RoomC); break;
	default:
		UE_LOG(LogTemp, Display, TEXT("All steps completed."));
		break;
	}
}

void URoomManager::StartAutoDemo(TSubclassOf<ARoomParent> RoomP, TSubclassOf<ARoomParent> RoomS,
	TSubclassOf<ARoomParent> RoomC, float StepDelaySeconds)
{
	// Store classes and timing
	AutoRoomP = RoomP;
	AutoRoomS = RoomS;
	AutoRoomC = RoomC;
	AutoDelay = FMath::Max(0.05f, StepDelaySeconds);

	// Clean reset of step-by-step state (and MST flags, etc.)
	ResetStepByStep();

	// Don't destroy what was just generated: start from current state (mega-triangle already created)
	bAutoDemo = true;

	// Repetitive timer: each tick advances one sub-step (exactly like manual clicking)
	GetWorld()->GetTimerManager().SetTimer(
		AutoDemoTimer, this, &URoomManager::AutoTick, AutoDelay, true);

	UE_LOG(LogTemp, Display, TEXT("[AutoDemo] START (delay=%.2fs)"), AutoDelay);
}

void URoomManager::StepByStepTriangulation(TSubclassOf<ARoomParent> RoomP)
{
	if (ActiveRunId != StepRunId)
	{
		ActiveRunId = StepRunId;
		CurrentStep = 0;
		CurrentLittleStep = 0;
		CurrentPointIndex = 0;
		RoomPrincipallist.Empty();
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), RoomP, RoomPrincipallist);
		FlushPersistentDebugLines(GetWorld());
	}
    if (RoomPrincipallist.Num() == 0)
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), RoomP, RoomPrincipallist);

    if (AllTriangles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Please create the mega triangle first!"));
        return;
    }

    if (CurrentPointIndex >= RoomPrincipallist.Num())
    {
        UE_LOG(LogTemp, Display, TEXT("Triangulation completed!"));
    	RemoveSuperTriangles();
        CurrentStep++;
    	TriangulationDone = true;
        CurrentPointIndex = 0;
        CurrentLittleStep = 0;
        return;
    }

    AActor* Room = RoomPrincipallist[CurrentPointIndex];
    FVector RoomLocation = Room->GetActorLocation();

    switch (CurrentLittleStep)
    {
        // 1️⃣ Choix du point
        case 0:
            ClearDrawAll();
    		RedrawStableState();
            DrawDebugSphere(GetWorld(), RoomLocation, 100, 16, FColor::Blue, true, -1);
            UE_LOG(LogTemp, Display, TEXT("[Step %d] Nouveau point (%.0f, %.0f)"), CurrentPointIndex, RoomLocation.X, RoomLocation.Y);
            CurrentLittleStep++;
            break;

    	// 2️⃣ Dessiner les cercles des triangles testés pour le point courant
	    case 1:
    		ClearDrawAll();
    		RedrawStableState();

    		for (const FTriangle& Tri : AllTriangles)
    		{
    			FVector Center;
    			if (Tri.CenterCircle(Center))
    			{
    				float R = Tri.GetRayon();
    				bool bInside = FVector::Dist(Center, RoomLocation) <= R;

    				DrawDebugCircle(
						GetWorld(),
						Center,
						R,
						64,
						bInside ? FColor::Red : FColor::Cyan, // rouge si point à l'intérieur, cyan sinon
						true,  // bPersistentLines
						-1,    // durée infinie
						0,
						20.f,
						FVector(1, 0, 0),
						FVector(0, 1, 0),
						false
					);
    			}
    		}

    		DrawDebugSphere(GetWorld(), RoomLocation, 100, 16, FColor::Blue, true, -1);
    		UE_LOG(LogTemp, Display, TEXT("[Step %d] Circles tested for current point displayed"), CurrentPointIndex);
    		CurrentLittleStep++;
    		break;

        // 3️⃣ Identify and mark invalid triangles
        case 2:
            ClearDrawAll();
    		RedrawStableState();
            TriangleErased.Empty();
            for (const FTriangle& Tri : AllTriangles)
            {
                FVector Center;
                if (Tri.CenterCircle(Center))
                {
                    if (FVector::Dist(Center, RoomLocation) <= Tri.GetRayon())
                    {
                        TriangleErased.Add(Tri);
                        Tri.DrawTriangle(GetWorld(), FColor::Red);
                    }
                    else
                    {
                        Tri.DrawTriangle(GetWorld(), FColor::Green);
                    }
                }
            }
            UE_LOG(LogTemp, Display, TEXT("[Step %d] %d invalid triangles detected"), CurrentPointIndex, TriangleErased.Num());
            CurrentLittleStep++;
            break;

        // 4️⃣ Remove bad triangles and create new ones
        case 3:
        {
            // Remove bad triangles
            for (const FTriangle& T : TriangleErased)
                AllTriangles.RemoveSingle(T);

            // Collect all edges
            TArray<FTriangleEdge> Edges;
            for (const FTriangle& T : TriangleErased)
                Edges.Append(T.GetEdges());

            // Identify unique edges (boundaries)
            TArray<FTriangleEdge> Boundary;
            for (int32 i = 0; i < Edges.Num(); ++i)
            {
                if (Edges[i].PointA.IsZero()) continue;
                int Count = 1;
                for (int32 j = i + 1; j < Edges.Num(); ++j)
                {
                    if (Edges[i] == Edges[j])
                    {
                        Count++;
                        Edges[j].PointA = FVector::ZeroVector;
                        Edges[j].PointB = FVector::ZeroVector;
                    }
                }
                if (Count == 1)
                {
                    Boundary.Add(Edges[i]);
                    Edges[i].DrawEdge(GetWorld(), FColor::Yellow);
                }
            }

            // Create new triangles
            for (const FTriangleEdge& Edge : Boundary)
            {
                FTriangle NewTri(RoomLocation, Edge.PointA, Edge.PointB);
                NewTri.DrawTriangle(GetWorld(), FColor::Green);
                AllTriangles.Add(NewTri);
            }

            UE_LOG(LogTemp, Display, TEXT("[Step %d] New triangles created: %d"), CurrentPointIndex, Boundary.Num());
            CurrentLittleStep = 0;
            CurrentPointIndex++;
            break;
        }
    }
}


void URoomManager::StepByStepPrim(TSubclassOf<ARoomParent> RoomP)
{
	ClearDrawAll();
	RedrawStableState();

	// Safety: rebuild MST if run changes, if not done, or if cleared
	if (FirstPath.Num() == 0 || !bMSTInitialized || ActiveRunId != StepRunId)
	{
		// ⚠️ Filter mega-triangle from path (see point 3 below)
		FirstPath.Empty();
		bMSTInitialized = false;

		CreatePath(RoomP);          // -> fills FirstPath
		bMSTInitialized = true;
	}

	for (const FTriangleEdge& Edge : FirstPath)
		Edge.DrawEdge(GetWorld(), FColor::Cyan);

	UE_LOG(LogTemp, Display, TEXT("[Prim] %d edges displayed."), FirstPath.Num());

	CurrentStep++;
	CurrentLittleStep = 0;
}

void URoomManager::StepByStepEvolvePath()
{
	ClearDrawAll();
	RedrawStableState();

	if (FirstPath.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FirstPath empty: run Prim first (next step)."));
		return;
	}

	EvolvePath();
	for (const FTriangleEdge& Edge : EvolvedPath)
		Edge.DrawEdge(GetWorld(), FColor::Green);

	UE_LOG(LogTemp, Display, TEXT("[Path] L-shaped paths drawn."));
	CurrentStep++;
}

void URoomManager::StepByStepClear(TSubclassOf<ARoomParent> RoomS)
{
	ClearSecondaryRoom(RoomS); // RoomS = secondary rooms
	UE_LOG(LogTemp, Display, TEXT("[Clear] Secondary rooms removed."));
	CurrentStep++;
}

void URoomManager::StepByStepCorridors(TSubclassOf<ARoomParent> RoomC)
{
	SpawnConnectionModules(RoomC);
	UE_LOG(LogTemp, Display, TEXT("[Corridor] Corridors created."));
	CurrentStep++;
}

void URoomManager::ResetStepByStep()
{
	CurrentStep = 0;
	CurrentLittleStep = 0;
	CurrentPointIndex = 0;

	bMSTInitialized = false;
	bPathsEvolved = false;

	// Always generate a new run ID
	StepRunId++;

	// IMPORTANT: Clear visual caches but don't destroy dungeon data
	FlushPersistentDebugLines(GetWorld());
}

void URoomManager::RedrawStableState()
{
	// Useful for repainting after flush: redraw what's already validated
	for (const FTriangle& T : AllTriangles)           T.DrawTriangle(GetWorld(), FColor(0,255,0));
	for (const FTriangleEdge& E : FirstPath)          E.DrawEdge(GetWorld(), FColor::Cyan);
	for (const FTriangleEdge& E : EvolvedPath)        E.DrawEdge(GetWorld(), FColor::Green);
}

