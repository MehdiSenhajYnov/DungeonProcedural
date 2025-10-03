// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/ConfigRoomDataAsset.h"
#include "GameFramework/Actor.h"
#include "ActorToGenerate.generated.h"


UCLASS()
class DUNGEONPROCEDURAL_API UAActorToGenerate : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	UAActorToGenerate();

	UFUNCTION(BlueprintCallable)
	static void GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes);
	
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

};
