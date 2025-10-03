// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomParent.generated.h"

class UBoxComponent;

UCLASS()
class DUNGEONPROCEDURAL_API ARoomParent : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARoomParent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Room Parameters")
	UBoxComponent* BoxCollision;
};
