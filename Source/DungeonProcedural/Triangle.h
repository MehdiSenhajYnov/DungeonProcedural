// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

USTRUCT()
struct FTriangle
{
	GENERATED_BODY()
	FTriangle(FVector pointA, FVector pointB, FVector pointC);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PointA;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector PointB;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector PointC;

	UFUNCTION(BlueprintCallable)
	FVector CenterCircle();

	UFUNCTION()
	float GetArea();

	UFUNCTION()
	float GetRayon();

	UFUNCTION()
	void DrawTriangle(const UWorld* InWorld);
};

USTRUCT()
struct FLine
{
	UPROPERTY()
	FVector Direction;

	UPROPERTY()
	FVector Point;

	UFUNCTION()
	bool Intersection(FLine LineChecked,FVector& OutCenter);
};
