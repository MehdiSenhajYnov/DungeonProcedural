// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/RoomManager.h"

#include "Components/BoxComponent.h"
#include "DungeonProcedural/Triangle.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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

void URoomManager::TriangulationStepByStep(TSubclassOf<ARoomParent> RoomP)
{
	if (CurrentLittleStep != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("CurrentLittleStep different de 0, on fini d'abords les petites étapes !"));
		TriangulationLittleStep(RoomP);
		return;
	}
	TArray<ToDrawCircle> AllToDraw;
	TArray<AActor*> RoomPrincipal;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),RoomP,RoomPrincipal);
	if (AllTriangles.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Faire le megatriangle avant !"));
		return;
	}
	FTriangle Megatriangle = AllTriangles.Last();

	if (!RoomPrincipal.IsValidIndex(CurrentStep))
	{
		UE_LOG(LogTemp, Warning, TEXT("Current Step pas bon"));
		return;
	}
	const AActor* Room = RoomPrincipal[CurrentStep];
	if (CurrentStep == 0)
	{
		FTriangle T1 = FTriangle(Room->GetActorLocation(), Megatriangle.PointA, Megatriangle.PointB);
		FTriangle T2 = FTriangle(Room->GetActorLocation(), Megatriangle.PointB, Megatriangle.PointC);
		FTriangle T3 = FTriangle(Room->GetActorLocation(), Megatriangle.PointC, Megatriangle.PointA);
		
		AllTriangles.Add(T1);
		AllTriangles.Add(T2);
		AllTriangles.Add(T3);

		LastTrianglesCreated.Add(T1);
		LastTrianglesCreated.Add(T2);
		LastTrianglesCreated.Add(T3);

	}
	else
	{
		// tracage les cercles circonscrit des differents triangles, et si le point choisi fait partie du cercle, on efface le triangle proprietaire du cercle
		for (FTriangle& TriangleToCheck : LastTrianglesCreated)
		{
			// dist centre circle et point
			// si il est plus grand ou plus petit que le rayon
			FVector CenterCircle;
			bool result = TriangleToCheck.CenterCircle(CenterCircle);
			if (!result)
			{
				continue;
			}
			float Dist = FMath::Abs((CenterCircle - Room->GetActorLocation()).Length());
			FVector CenterCircleToDraw;
			bool CenterCircleIsGood = TriangleToCheck.CenterCircle(CenterCircleToDraw);
			if (CenterCircleIsGood)
			{
				AllToDraw.Add(ToDrawCircle(CenterCircleToDraw,TriangleToCheck.GetRayon()));
			} else
			{
				UE_LOG(LogTemp, Warning, TEXT("Center circle is not good"));
			}
			if (Dist <= TriangleToCheck.GetRayon())
			{
				AllTriangles.Remove(TriangleToCheck);
				TriangleErased.Add(TriangleToCheck);
			}
		}
		LastTrianglesCreated.Empty();
		
		// ALGORITHME BOWYER-WATSON CORRECT
		// 1. Collecter toutes les edges des triangles supprimés
		TArray<FTriangleEdge> AllEdges;
		for (FTriangle& ErasedTriangle : TriangleErased)
		{
			TArray<FTriangleEdge> TriangleEdges = ErasedTriangle.GetEdges();
			AllEdges.Append(TriangleEdges);
		}

		// 2. Identifier les edges de boundary (qui apparaissent exactement une fois)
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
		
		TriangleErased.Empty();

		// 3. Créer de nouveaux triangles en connectant le point à chaque edge de boundary
		TArray<FTriangle> TrianglesToAdd;
		for (const FTriangleEdge& Edge : BoundaryEdges)
		{
			FTriangle NewTriangle(Room->GetActorLocation(), Edge.PointA, Edge.PointB);
			TrianglesToAdd.Add(NewTriangle);
		}


		// Ajouts des triangles a la liste
		for (FTriangle& TriangleToAdd : TrianglesToAdd)
		{
			TriangleToAdd.DrawTriangle(GetWorld());
			AllTriangles.AddUnique(TriangleToAdd);
			LastTrianglesCreated.Add(TriangleToAdd);
		}
	}
	
	CurrentStep +=1;
	
	DrawAll();
	DrawDebugSphere(GetWorld(),Room->GetActorLocation(),50,50,FColor(0,0,0),true,-1,0 , 50);
	for (auto toDraw : AllToDraw)
	{
		DrawDebugCircle(GetWorld(), toDraw.Center, toDraw.radius, 50, FColor::Blue, true, -1, 0, 50, FVector(0,1,0), FVector(1,0,0));
	}

	UE_LOG(LogTemp, Display, TEXT("FINI!"))
}

void URoomManager::TriangulationLittleStep(TSubclassOf<ARoomParent> RoomP)
{
	TArray<ToDrawCircle> AllToDraw;
	TArray<AActor*> RoomPrincipal;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),RoomP,RoomPrincipal);
	if (AllTriangles.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Faire le megatriangle avant !"));
		return;
	}
	FTriangle Megatriangle = AllTriangles.Last();

	if (!RoomPrincipal.IsValidIndex(CurrentStep))
	{
		UE_LOG(LogTemp, Warning, TEXT("Current Step pas bon"));
		return;
	}
	const AActor* Room = RoomPrincipal[CurrentStep];

	UE_LOG(LogTemp, Display, TEXT("Etape : %d.%d!"), CurrentStep, CurrentLittleStep)
	
	// Premiere Etape pour montrer le choix de la room
	if (CurrentLittleStep == 0)
	{
		CurrentLittleStep += 1;
	}
	else
	{
		if (CurrentStep == 0)
		{

				
			if (CurrentLittleStep == 1)
			{
				CurrentLittleStep +=1;
				FTriangle T1 = FTriangle(Room->GetActorLocation(), Megatriangle.PointA, Megatriangle.PointB);
				FTriangle T2 = FTriangle(Room->GetActorLocation(), Megatriangle.PointB, Megatriangle.PointC);
				FTriangle T3 = FTriangle(Room->GetActorLocation(), Megatriangle.PointC, Megatriangle.PointA);
				
				LastTrianglesCreated.Add(T1);
				LastTrianglesCreated.Add(T2);
				LastTrianglesCreated.Add(T3);
		
			} else if (CurrentLittleStep == 2)
			{
				CurrentLittleStep = 0;
				CurrentStep +=1;
				if (LastTrianglesCreated.Num() >= 3)
				{
					AllTriangles.Add(LastTrianglesCreated[LastTrianglesCreated.Num() - 1]);
					AllTriangles.Add(LastTrianglesCreated[LastTrianglesCreated.Num() - 2]);
					AllTriangles.Add(LastTrianglesCreated[LastTrianglesCreated.Num() - 3]);
				}
			}
			
			for (FTriangle& TriangleToCheck : LastTrianglesCreated)
			{
				FVector CenterCircle;
				bool result = TriangleToCheck.CenterCircle(CenterCircle);
				if (!result)
				{
					continue;
				}
				FVector CenterCircleToDraw;
				bool CenterCircleIsGood = TriangleToCheck.CenterCircle(CenterCircleToDraw);
				if (CenterCircleIsGood)
				{
					AllToDraw.Add(ToDrawCircle(CenterCircleToDraw,TriangleToCheck.GetRayon()));
				} else
				{
					UE_LOG(LogTemp, Warning, TEXT("Center circle is not good"));
				}
				
			}

			


		}
		else
		{
			if (CurrentLittleStep == 1)
			{
				
				// tracage les cercles circonscrit des differents triangles
				for (FTriangle& TriangleToCheck : LastTrianglesCreated)
				{
					FVector CenterCircle;
					bool result = TriangleToCheck.CenterCircle(CenterCircle);
					if (!result)
					{
						continue;
					}
					FVector CenterCircleToDraw;
					if (TriangleToCheck.CenterCircle(CenterCircleToDraw))
					{
						AllToDraw.Add(ToDrawCircle(CenterCircleToDraw,TriangleToCheck.GetRayon()));
					} else
					{
						UE_LOG(LogTemp, Warning, TEXT("Center circle is not good"));
					}

				}
				CurrentLittleStep += 1;
			}
			else if (CurrentLittleStep == 2)
			{
				// tracage les cercles circonscrit des differents triangles, et si le point choisi fait partie du cercle, on efface le triangle proprietaire du cercle
				for (FTriangle& TriangleToCheck : LastTrianglesCreated)
				{
					// dist centre circle et point
					// si il est plus grand ou plus petit que le rayon
					FVector CenterCircle;
					bool result = TriangleToCheck.CenterCircle(CenterCircle);
					if (!result)
					{
						continue;
					}
					float Dist = FMath::Abs((CenterCircle - Room->GetActorLocation()).Length());
					FVector CenterCircleToDraw;
					bool CenterCircleIsGood = TriangleToCheck.CenterCircle(CenterCircleToDraw);
					if (CenterCircleIsGood)
					{
						AllToDraw.Add(ToDrawCircle(CenterCircleToDraw,TriangleToCheck.GetRayon()));
					} else
					{
						UE_LOG(LogTemp, Warning, TEXT("Center circle is not good"));
					}
					if (Dist <= TriangleToCheck.GetRayon() && TriangleToCheck != Megatriangle)
					{
						AllTriangles.Remove(TriangleToCheck);
						TriangleErased.AddUnique(TriangleToCheck);
					}
				}
				LastTrianglesCreated.Empty();

				CurrentLittleStep += 1;
			}			
			else if (CurrentLittleStep == 3)
			{

				// ALGORITHME BOWYER-WATSON CORRECT
				// 1. Collecter toutes les edges des triangles supprimés
				TArray<FTriangleEdge> AllEdges;
				for (FTriangle& ErasedTriangle : TriangleErased)
				{
					TArray<FTriangleEdge> TriangleEdges = ErasedTriangle.GetEdges();
					AllEdges.Append(TriangleEdges);
				}

				// 2. Identifier les edges de boundary (qui apparaissent exactement une fois)
				TArray<FTriangleEdge> BoundaryEdges;
				for (int32 i = 0; i < AllEdges.Num(); ++i)
				{
					if (AllEdges[i].PointA.IsZero() && AllEdges[i].PointB.IsZero()) 
						continue; // Skip edges marquées comme supprimées
					
					int32 Count = 1;
					// Compter combien de fois cette edge apparaît
					for (int32 j = i + 1; j < AllEdges.Num(); ++j)
					{
						if (AllEdges[i] == AllEdges[j])
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
						BoundaryEdges.Add(AllEdges[i]);
					}
				}
				
				TriangleErased.Empty();

				// 3. Créer de nouveaux triangles en connectant le point à chaque edge de boundary
				TArray<FTriangle> TrianglesToAdd;
				for (const FTriangleEdge& Edge : BoundaryEdges)
				{
					FTriangle NewTriangle(Room->GetActorLocation(), Edge.PointA, Edge.PointB);
					TrianglesToAdd.Add(NewTriangle);
				}


				// Ajouts des triangles a la liste
				for (FTriangle& TriangleToAdd : TrianglesToAdd)
				{
					TriangleToAdd.DrawTriangle(GetWorld());
					AllTriangles.AddUnique(TriangleToAdd);
					LastTrianglesCreated.AddUnique(TriangleToAdd);
				}

				CurrentLittleStep = 0;
				CurrentStep +=1;
				
			}
			
		}
		
	}
	
	
	DrawAll();
	DrawDebugSphere(GetWorld(),Room->GetActorLocation(),50,50,FColor(0,0,0),true,-1,0 , 50);
	for (auto toDraw : AllToDraw)
	{
		DrawDebugCircle(GetWorld(), toDraw.Center, toDraw.radius, 50, FColor::Blue, true, -1, 0, 50, FVector(0,1,0), FVector(1,0,0));
	}

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

#pragma optimize("", on)
