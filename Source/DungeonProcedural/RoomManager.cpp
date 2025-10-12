// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/RoomManager.h"

#include "Components/BoxComponent.h"
#include "DungeonProcedural/Triangle.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Box.h"
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

#pragma optimize("", off)

void URoomManager::GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes)
{
	ClearAll();	
	float MaxProba = 0;
	for (FRoomType& RoomType : RoomTypes)
	{
		MaxProba += RoomType.Probability;
	}

	
	for (int i = 0; i < NbRoom; ++i)
	{
		float CurrentProba = FMath::FRandRange(0,MaxProba);
		
		float currentMaxProbability = 0;
		FRoomType* RoomType = RoomTypes.FindByPredicate([CurrentProba, &currentMaxProbability](const FRoomType& Room){
			currentMaxProbability += Room.Probability;
			return CurrentProba < currentMaxProbability;
		});

		if (RoomType != nullptr){
			AActor* SpawnedActorRaw = GetWorld()->SpawnActor(RoomType->TypeOfRoomToSpawn);

			FVector RandomScale;
			RandomScale.X = FMath::FRandRange(RoomType->SizeMin, RoomType->SizeMax);
			RandomScale.Y = FMath::FRandRange(RoomType->SizeMin, RoomType->SizeMax);
			RandomScale.Z = 1;
			SpawnedActorRaw->SetActorScale3D(RandomScale);

			
			if (SpawnedActorRaw->IsA(ARoomParent::StaticClass()))
			{
				SpawnedActors.Add(Cast<ARoomParent>(SpawnedActorRaw));
			}
		}
	}
	ResolveRoomOverlaps(SpawnedActors);
}

void URoomManager::MegaTriangle(TSubclassOf<ARoomParent> Room)
{
	if (SpawnedActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pas d'acteurs pour créer le méga-triangle!"));
		return;
	}
	
	// Initialiser avec le premier point
	float MaxX = SpawnedActors[0]->GetActorLocation().X;
	float MaxY = SpawnedActors[0]->GetActorLocation().Y;
	float MinX = MaxX;
	float MinY = MaxY;
	
	// Trouver les vrais min/max
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
	
	// Stocker les positions du méga-triangle
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
		UE_LOG(LogTemp, Warning, TEXT("Faire le megatriangle avant !"));
		return;
	}
	
	// Boucle sur TOUS les points à insérer
	for (AActor* Room : RoomPrincipal)
	{
		FVector RoomLocation = Room->GetActorLocation();
		
		// ÉTAPE 1: Trouver TOUS les triangles dont le circumcircle contient le nouveau point
		TArray<FTriangle> BadTriangles;
		
		// IMPORTANT: Parcourir TOUS les triangles existants, pas seulement les derniers créés!
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
		
		// ÉTAPE 2: Supprimer tous les bad triangles
		for (const FTriangle& BadTriangle : BadTriangles)
		{
			AllTriangles.RemoveSingle(BadTriangle);
		}
		
		// ÉTAPE 3: Collecter toutes les edges des triangles supprimés
		TArray<FTriangleEdge> AllEdges;
		for (const FTriangle& BadTriangle : BadTriangles)
		{
			TArray<FTriangleEdge> TriangleEdges = BadTriangle.GetEdges();
			AllEdges.Append(TriangleEdges);
		}

		// ÉTAPE 4: Identifier les edges de boundary (qui apparaissent exactement une fois)
		TArray<FTriangleEdge> BoundaryEdges;
		for (int32 EdgeIdx = 0; EdgeIdx < AllEdges.Num(); ++EdgeIdx)
		{
			if (AllEdges[EdgeIdx].PointA.IsZero() && AllEdges[EdgeIdx].PointB.IsZero()) 
				continue; // Skip edges marquées comme supprimées
			
			int32 Count = 1;
			// Compter combien de fois cette edge apparaît
			for (int32 j = EdgeIdx + 1; j < AllEdges.Num(); ++j)
			{
				if (AllEdges[EdgeIdx] == AllEdges[j])
				{
					Count++;
					// Marquer l'edge comme traitée
					AllEdges[j].PointA = FVector::ZeroVector;
					AllEdges[j].PointB = FVector::ZeroVector;
				}
			}
			
			// Si l'edge apparaît exactement une fois, c'est une edge de boundary
			if (Count == 1)
			{
				BoundaryEdges.Add(AllEdges[EdgeIdx]);
			}
		}

		// ÉTAPE 5: Créer de nouveaux triangles en connectant le point à chaque edge de boundary
		for (const FTriangleEdge& Edge : BoundaryEdges)
		{
			FTriangle NewTriangle(RoomLocation, Edge.PointA, Edge.PointB);
			AllTriangles.Add(NewTriangle);
		}
		
		DrawDebugSphere(GetWorld(), RoomLocation, 50, 50, FColor(0,0,0), true, -1, 0, 50);
	}
	
	// Supprimer les triangles connectés au méga-triangle
	RemoveSuperTriangles();
	
	DrawAll();

	UE_LOG(LogTemp, Display, TEXT("FINI!"))
}

void URoomManager::CreatePath(TSubclassOf<ARoomParent> RoomP)
{
	ClearDrawAll();
	TArray<AActor*> RoomPrincipal;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),RoomP,RoomPrincipal);

	if (AllTriangles.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Faire le megatriangle avant !"));
		return;
	}

	TSet<FVector> PointAlreadyUsed;
	
	TSet<FTriangleEdge> AllEdges;

	TMap<float,TSet<FTriangleEdge>> PointsSortedByDistance;
	FVector RoomLocation = RoomPrincipal[0]->GetActorLocation();

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
	

	for (const FTriangleEdge& singlePath : FirstPath)
	{
		singlePath.DrawEdge(GetWorld());
	}
	
}

void URoomManager::EvolvePath()
{
	ClearDrawAll();
	for (FTriangleEdge EdgeToCheck : FirstPath)
	{
		if (EdgeToCheck.IsStraightLine())
		{
			EvolvedPath.Add(EdgeToCheck);
		}
		else
		{
			FVector FirstIntersection(EdgeToCheck.PointA.X, EdgeToCheck.PointB.Y,0);
			
			FTriangleEdge ToAdd1 = FTriangleEdge(EdgeToCheck.PointA, FirstIntersection);
			FTriangleEdge ToAdd2 = FTriangleEdge(FirstIntersection, EdgeToCheck.PointB);

			FVector SecondIntersection(EdgeToCheck.PointB.X, EdgeToCheck.PointA.Y,0);
			FTriangleEdge ToAdd3 = FTriangleEdge(EdgeToCheck.PointA, SecondIntersection);
			FTriangleEdge ToAdd4 = FTriangleEdge(SecondIntersection, EdgeToCheck.PointB);

			EvolvedPath.Add(ToAdd1);
			EvolvedPath.Add(ToAdd2);
			EvolvedPath.Add(ToAdd3);
			EvolvedPath.Add(ToAdd4);
		}
	}

	for (const auto& Edge : EvolvedPath)
	{
		Edge.DrawEdge(GetWorld());
	}

}

void URoomManager::ClearSecondaryRoom(TSubclassOf<ARoomParent> SecondaryRoomType)
{
	TArray<AActor*> AllSecondaryRoom;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),SecondaryRoomType,AllSecondaryRoom);

	if (AllTriangles.Num() <= 0)
	{
		return;
	}

	// Vérifier si EvolvedPath est vide
	if (EvolvedPath.Num() == 0)
	{
		return;
	}

	for (int SecondaryRoomIndex = 0; SecondaryRoomIndex < AllSecondaryRoom.Num(); ++SecondaryRoomIndex)
	{
		AActor* SecondaryRoom = AllSecondaryRoom[SecondaryRoomIndex];
		if (!SecondaryRoom) continue;
		
		ARoomParent* RoomParent = Cast<ARoomParent>(SecondaryRoom);
		if (!RoomParent || !RoomParent->BoxCollision) 
		{
			continue;
		}
		
		// Utiliser la taille réelle de la BoxCollision
		FVector BoxExtent = RoomParent->BoxCollision->GetScaledBoxExtent();
		FVector RoomLocation = SecondaryRoom->GetActorLocation();
		
		bool IsInPath = false;
		
		// Vérifier si la room est intersectée par AU MOINS UN segment du chemin
		for (const FTriangleEdge& EdgeToCheck : EvolvedPath)
		{
			// Utiliser BoxExtent * 2 pour avoir la taille totale de la boîte
			if (IsSegmentIntersectingBox(EdgeToCheck.PointA, EdgeToCheck.PointB, RoomLocation, BoxExtent * 2.0f))
			{
				IsInPath = true;
				break;
			}
		}
		
		// Détruire la room seulement si elle n'est PAS dans le chemin
		if (!IsInPath)
		{
			SecondaryRoom->Destroy();
		}

	}

}
// Width = X, Height = Z, Depth = Y
bool URoomManager::IsSegmentIntersectingBox(const FVector& PointA, const FVector& PointB, const FVector& CenterOfBox, FVector Size)
{
	FVector Extent = Size* 0.5;
	FBox Box(CenterOfBox - Extent, CenterOfBox + Extent);

	// Vérifie si un des points est à l'intérieur
	if (Box.IsInside(PointA) || Box.IsInside(PointB))
		return true;

	float tmin = 0.0f;
	float tmax = 1.0f;

	FVector d = PointB - PointA;

	for (int i = 0; i < 3; i++)
	{
		if (FMath::Abs(d[i]) < KINDA_SMALL_NUMBER)
		{
			// Segment parallèle aux faces
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

// TEST Optimisation

// Fonction principale de résolution des collisions
void URoomManager::ResolveRoomOverlaps(TArray<ARoomParent*>& SpawnedActorsRaw)
{
    if (SpawnedActorsRaw.Num() == 0) return;

	// peut etre a enlever, sert a randomizer la liste, ce qui normalement devrait deja etre le cas, mais avec ca on dirait c'est plus "random" dans le resultat
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
		            
						float MoveDistance = 50.0f;
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
	// Supprimer tous les triangles qui partagent un sommet avec le méga-triangle
	AllTriangles.RemoveAll([this](const FTriangle& Triangle) {
		TArray<FVector> Points = Triangle.GetAllPoints();
		for (const FVector& Point : Points)
		{
			if (Point.Equals(MegaTrianglePointA, 1.0f) || 
				Point.Equals(MegaTrianglePointB, 1.0f) || 
				Point.Equals(MegaTrianglePointC, 1.0f))
			{
				return true; // Supprimer ce triangle
			}
		}
		return false; // Garder ce triangle
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

#pragma optimize("", on)
