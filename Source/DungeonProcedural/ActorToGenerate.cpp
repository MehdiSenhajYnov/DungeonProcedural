// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/ActorToGenerate.h"

#include "Components/BoxComponent.h"

#pragma optimize("", off)
// Sets default values
UAActorToGenerate::UAActorToGenerate()
{
}



void UAActorToGenerate::GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes)
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
			AActor* SpawnedActorRaw = GWorld->SpawnActor(RoomType->TypeOfRoomToSpawn);
			if (SpawnedActorRaw->IsA(ARoomParent::StaticClass()))
			{
				SpawnedActors.Add(Cast<ARoomParent>(SpawnedActorRaw));
			}
		}
	}

	for (ARoomParent* SpawnedActor : SpawnedActors)
	{
		SpawnedActor->BoxCollision->SetSimulatePhysics(true);
	}
}

void UAActorToGenerate::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent){
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// PropertyChangedEvent.MemberProperty->GetName() == GET_MEMBER_NAME_CHECKED(UAActorToGenerate, RoomTypes);
}

#pragma optimize("", on)