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
	UE_LOG(LogTemp, Display, TEXT("=== [Delaunay Triangles] ==="));
	for (const FTriangle& Tri : AllTriangles)
	{
		UE_LOG(LogTemp, Display, TEXT("Triangle: (%.0f,%.0f) | (%.0f,%.0f) | (%.0f,%.0f)"),
			Tri.PointA.X, Tri.PointA.Y,
			Tri.PointB.X, Tri.PointB.Y,
			Tri.PointC.X, Tri.PointC.Y);
	}
	UE_LOG(LogTemp, Display, TEXT("=============================="));
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
	// Nettoyage des anciens tracés
	ClearDrawAll();
	EvolvedPath.Empty();

	// Pour chaque connexion entre deux salles principales
	for (const FTriangleEdge& EdgeToCheck : FirstPath)
	{
		// Si les deux salles sont déjà alignées (horizontal ou vertical)
		if (EdgeToCheck.IsStraightLine())
		{
			EvolvedPath.Add(EdgeToCheck);
		}
		else
		{
			// On doit créer un chemin en L
			FVector Intersection;

			// Deux possibilités pour créer le "L" :
			// - (A.x, B.y) : on part d'abord horizontalement, puis verticalement
			// - (B.x, A.y) : on part d'abord verticalement, puis horizontalement
			// On en choisit UNE seule (aléatoirement pour varier la génération)
			if (FMath::RandBool())
				Intersection = FVector(EdgeToCheck.PointA.X, EdgeToCheck.PointB.Y, 0);
			else
				Intersection = FVector(EdgeToCheck.PointB.X, EdgeToCheck.PointA.Y, 0);

			// Ajout des deux segments composant le "L"
			EvolvedPath.Add(FTriangleEdge(EdgeToCheck.PointA, Intersection));
			EvolvedPath.Add(FTriangleEdge(Intersection, EdgeToCheck.PointB));
		}
	}

	// Dessin de tous les chemins générés pour visualisation dans l'éditeur
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

	// Avance d’un cran dans la démo (réutilise ta logique manuelle)
	StepByStep(AutoRoomP, AutoRoomS, AutoRoomC);

	// Condition d’arrêt : toutes les grandes étapes sont finies
	// (0:Triang, 1:Prim, 2:L, 3:Clear, 4:Corridors)
	if (CurrentStep > 4)
	{
		StopAutoDemo();
		UE_LOG(LogTemp, Display, TEXT("Completed all steps."));
	}

	// Protection : si jamais quelqu’un "ClearAll" en plein milieu, on stoppe proprement
	// (tu peux garder ou retirer, c’est juste une sécurité)
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

void URoomManager::SpawnConnectionModules(TSubclassOf<AActor> CorridorBP)
{
	ClearDrawAll();
	if (EvolvedPath.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pas de chemins évolués pour créer les modules de connexion."));
		return;
	}

	for (const FTriangleEdge& Edge : EvolvedPath)
	{
		FVector Start = Edge.PointA;
		FVector End = Edge.PointB;

		FVector Direction = End - Start;
		float Length = Direction.Length();
		Direction.Normalize();

		// Calcul de la position médiane pour placer le couloir
		FVector Middle = Start + (Direction * (Length / 2));

		// Calcul de la rotation
		FRotator Rotation = Direction.Rotation();

		// Spawn du couloir
		FActorSpawnParameters SpawnParams;
		AActor* Corridor = GetWorld()->SpawnActor<AActor>(CorridorBP, Middle, Rotation, SpawnParams);
		if (!Corridor) continue;

		// Mise à l’échelle selon la longueur du segment
		FVector Scale = Corridor->GetActorScale3D();
		Scale.X = Length / 100.0f; // Ajuste le "100" selon la longueur d’un mesh de base
		Corridor->SetActorScale3D(Scale);

		// Optionnel : garde une référence pour cleanup plus tard
		OtherActorsToClear.Add(Corridor);
	}

	UE_LOG(LogTemp, Display, TEXT("✅ Modules de connexion générés (%d segments)."), EvolvedPath.Num());
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
		UE_LOG(LogTemp, Display, TEXT("Toutes les étapes sont terminées."));
		break;
	}
}

void URoomManager::StartAutoDemo(TSubclassOf<ARoomParent> RoomP, TSubclassOf<ARoomParent> RoomS,
	TSubclassOf<ARoomParent> RoomC, float StepDelaySeconds)
{
	// garde les classes et le tempo
	AutoRoomP = RoomP;
	AutoRoomS = RoomS;
	AutoRoomC = RoomC;
	AutoDelay = FMath::Max(0.05f, StepDelaySeconds);

	// reset propre du step-by-step (et flags MST, etc.)
	ResetStepByStep();

	// on NE détruit pas ce que tu viens de générer : on part de l’état courant (méga-triangle déjà fait)
	bAutoDemo = true;

	// timer répétitif : chaque tick avance d’une sous-étape (exactement comme si tu cliquais)
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
        UE_LOG(LogTemp, Warning, TEXT("Veuillez créer le méga triangle avant !"));
        return;
    }

    if (CurrentPointIndex >= RoomPrincipallist.Num())
    {
        UE_LOG(LogTemp, Display, TEXT("Triangulation terminée !"));
    	RemoveSuperTriangles();
        CurrentStep++;
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
    		UE_LOG(LogTemp, Display, TEXT("[Step %d] Cercles testés pour le point courant affichés"), CurrentPointIndex);
    		CurrentLittleStep++;
    		break;

        // 3️⃣ Identifier et marquer les triangles invalides
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
            UE_LOG(LogTemp, Display, TEXT("[Step %d] %d triangles invalides détectés"), CurrentPointIndex, TriangleErased.Num());
            CurrentLittleStep++;
            break;

        // 4️⃣ Supprimer les bad triangles + créer les nouveaux
        case 3:
        {
            // Supprimer les bad triangles
            for (const FTriangle& T : TriangleErased)
                AllTriangles.RemoveSingle(T);

            // Récupérer toutes les edges
            TArray<FTriangleEdge> Edges;
            for (const FTriangle& T : TriangleErased)
                Edges.Append(T.GetEdges());

            // Identifier les edges uniques (frontières)
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

            // Créer les nouveaux triangles
            for (const FTriangleEdge& Edge : Boundary)
            {
                FTriangle NewTri(RoomLocation, Edge.PointA, Edge.PointB);
                NewTri.DrawTriangle(GetWorld(), FColor::Green);
                AllTriangles.Add(NewTri);
            }

            UE_LOG(LogTemp, Display, TEXT("[Step %d] Nouveaux triangles créés : %d"), CurrentPointIndex, Boundary.Num());
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

	// Sécurité : (re)construire le MST si on change de run, si jamais pas fait, ou si vidé.
	if (FirstPath.Num() == 0 || !bMSTInitialized || ActiveRunId != StepRunId)
	{
		// ⚠️ filtre méga-triangle côté path (voir point 3 plus bas)
		FirstPath.Empty();
		bMSTInitialized = false;

		CreatePath(RoomP);          // -> remplit FirstPath
		bMSTInitialized = true;
	}

	for (const FTriangleEdge& Edge : FirstPath)
		Edge.DrawEdge(GetWorld(), FColor::Cyan);

	UE_LOG(LogTemp, Display, TEXT("[Prim] %d arêtes affichées."), FirstPath.Num());

	CurrentStep++;
	CurrentLittleStep = 0;
}

void URoomManager::StepByStepEvolvePath()
{
	ClearDrawAll();
	RedrawStableState();

	if (FirstPath.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FirstPath vide : lance Prim avant (Step suivant)."));
		return;
	}

	EvolvePath();
	for (const FTriangleEdge& Edge : EvolvedPath)
		Edge.DrawEdge(GetWorld(), FColor::Green);

	UE_LOG(LogTemp, Display, TEXT("[Path] Chemins en L dessinés."));
	CurrentStep++;
}

void URoomManager::StepByStepClear(TSubclassOf<ARoomParent> RoomS)
{
	ClearSecondaryRoom(RoomS); // RoomC = salles secondaires
	UE_LOG(LogTemp, Display, TEXT("[Clear] Salles secondaires supprimées."));
	CurrentStep++;
}

void URoomManager::StepByStepCorridors(TSubclassOf<ARoomParent> RoomC)
{
	SpawnConnectionModules(RoomC);
	UE_LOG(LogTemp, Display, TEXT("[Corridor] Corridors créés."));
	CurrentStep++;
}

void URoomManager::ResetStepByStep()
{
	CurrentStep = 0;
	CurrentLittleStep = 0;
	CurrentPointIndex = 0;

	bMSTInitialized = false;
	bPathsEvolved = false;

	// on relancera toujours un nouveau run id
	StepRunId++;

	// IMPORTANT : on vide les caches visuels mais on ne détruit pas la data du donjon ici
	FlushPersistentDebugLines(GetWorld());
}

void URoomManager::RedrawStableState()
{
	// utile si tu veux “repeindre” après un flush : redessine ce qui est déjà validé
	for (const FTriangle& T : AllTriangles)           T.DrawTriangle(GetWorld(), FColor(0,255,0));
	for (const FTriangleEdge& E : FirstPath)          E.DrawEdge(GetWorld(), FColor::Cyan);
	for (const FTriangleEdge& E : EvolvedPath)        E.DrawEdge(GetWorld(), FColor::Green);
}

#pragma optimize("", on)
