// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonProcedural/RoomParent.h"

#include "Components/BoxComponent.h"


// Sets default values
ARoomParent::ARoomParent()
{
	// Disable tick for performance - rooms are static after generation
	PrimaryActorTick.bCanEverTick = true;

	// Create box collision component for overlap detection and room spacing
	BoxCollision = CreateDefaultSubobject<UBoxComponent>("BoxCollision");
	RootComponent = BoxCollision;
}


