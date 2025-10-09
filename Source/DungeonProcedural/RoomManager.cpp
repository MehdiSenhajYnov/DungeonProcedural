// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/RoomManager.h"

#include "Components/BoxComponent.h"
#include "DungeonProcedural/Triangle.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#pragma optimize("", off)

void URoomManager::GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes)
{
	SpawnedActors.Empty();
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
	float MaxX = 0;
	float MaxY = 0;
	float MinX = 0;
	float MinY = 0;
	
	for (ARoomParent* Room : SpawnedActors)
	{
		if ( Room->GetActorLocation().X > MaxX )
		{
			MaxX = Room->GetActorLocation().X;
		}
		else if ( Room->GetActorLocation().X < MinX)
		{
			MinX = Room->GetActorLocation().X;
		}
		if ( Room->GetActorLocation().Y > MaxY)
		{
			MaxY = Room->GetActorLocation().Y;
		}
		else if ( Room->GetActorLocation().Y < MinY)
		{
			MinY = Room->GetActorLocation().Y;
		}
	}

	float avgX = (MaxX + MinX) / 2;
	float avgY = (MaxY + MinY) / 2;
	float difX = FMath::Abs(MaxX - avgX);
	float difY = FMath::Abs(MaxY - avgY);
	
	
	
	AActor* UpTriangle = GetWorld()->SpawnActor(Room);
	UpTriangle->SetActorLocation(FVector(avgX,avgY+difY*2,0));
	AActor* LeftTriangle = GetWorld()->SpawnActor(Room);
	LeftTriangle->SetActorLocation(FVector(avgX-difX*4,avgY-difY*2,0));
	AActor* RightTriangle = GetWorld()->SpawnActor(Room);
	RightTriangle->SetActorLocation(FVector(avgX+difX*4,avgY-difY*2,0));
	
	AllTriangles.Add(FTriangle(UpTriangle->GetActorLocation(),LeftTriangle->GetActorLocation(),UpTriangle->GetActorLocation()));
}

void URoomManager::Triangulation(TSubclassOf<ARoomParent> RoomP)
{
	TArray<AActor*> RoomPrincipal;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),RoomP,RoomPrincipal);
	for (const AActor* Room : RoomPrincipal)
	{
		DrawDebugSphere(GetWorld(),Room->GetActorLocation(),50,50,FColor(0,0,0),true,-1,0 , 50);
		
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

#pragma optimize("", on)