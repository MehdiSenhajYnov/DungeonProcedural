// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Triangle.generated.h"

USTRUCT(BlueprintType)
struct FTriangle
{
	GENERATED_BODY()
	FTriangle() = default;
	FTriangle(FVector pointA, FVector pointB, FVector pointC);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PointA;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector PointB;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector PointC;

	FVector CenterCircle();

	float GetArea();

	float GetRayon();

	void DrawTriangle(const UWorld* InWorld);
};

USTRUCT(BlueprintType)
struct FLine
{
	GENERATED_BODY()
	UPROPERTY()
	FVector Direction;

	UPROPERTY()
	FVector Point;

	bool Intersection(FLine LineChecked,FVector& OutCenter);
};
