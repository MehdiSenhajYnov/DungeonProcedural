// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/RoomManager.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

#pragma optimize("", off)

void URoomManager::GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes)
{
	float MaxProba = 0;
	for (FRoomType& RoomType : RoomTypes)
	{
		MaxProba += RoomType.Probability;
	}

	TArray<ARoomParent*> SpawnedActors;
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

	for (ARoomParent* SpawnedActor : SpawnedActors)
	{
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));

		TArray<AActor*> ActorToIgnore;
		ActorToIgnore.Add(SpawnedActor);

		TArray<AActor*> OutActors;

		UKismetSystemLibrary::BoxOverlapActors(GetWorld(), SpawnedActor->BoxCollision->GetComponentLocation(),
			SpawnedActor->BoxCollision->GetScaledBoxExtent() * 2, ObjectTypes, ARoomParent::StaticClass(),ActorToIgnore, OutActors);
		while (OutActors.Num() > 0)
		{
			FVector Direction = FMath::VRand();
			Direction.Z = 0;
			SpawnedActor->AddActorLocalOffset(Direction * 1.0f);
			
			OutActors.Empty();
			UKismetSystemLibrary::BoxOverlapActors(GetWorld(), SpawnedActor->BoxCollision->GetComponentLocation()	,
			SpawnedActor->BoxCollision->GetScaledBoxExtent() * 2, ObjectTypes, ARoomParent::StaticClass(),ActorToIgnore, OutActors);
		}
	}
}
#pragma optimize("", on)