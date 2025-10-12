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

	bool operator==(const FTriangle& Other) const
	{
		return GetAllPoints().Contains(Other.PointA) && GetAllPoints().Contains(Other.PointB) && GetAllPoints().Contains(Other.PointC);
	}

	bool operator!=(const FTriangle& Other) const
	{
		return !(*this == Other);
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PointA;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector PointB;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector PointC;

	TArray<FVector> GetAllPoints() const;

	TArray<FTriangleEdge> GetEdges() const;

	bool CenterCircle(FVector& outCenter) const;

	float GetArea() const;

	float GetRayon() const;

	void DrawTriangle(const UWorld* InWorld, FColor ColorToUse = FColor(0,255,0)) const;
private:
	TArray<FVector> points;
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

// Structure pour représenter une arête entre deux points
USTRUCT(BlueprintType)
struct FTriangleEdge
{
	GENERATED_BODY()
	
	FTriangleEdge() = default;
	FTriangleEdge(const FVector& A, const FVector& B) : PointA(A), PointB(B) {}
	
	UPROPERTY()
	FVector PointA;
	
	UPROPERTY()
	FVector PointB;

	float GetLength() const;

	void DrawEdge(const UWorld* InWorld, FColor ColorToUse = FColor(0,255,0)) const;

	bool IsStraightLine(float Tolerance = 50) const
	{
		if (FMath::IsNearlyEqual(PointA.X, PointB.X, Tolerance))
		{
			return true;
		}
		if (FMath::IsNearlyEqual(PointA.Y, PointB.Y, Tolerance))
		{
			return true;
		}
		return false;
	}
	
	bool operator==(const FTriangleEdge& Other) const
	{
		return (PointA.Equals(Other.PointA) && PointB.Equals(Other.PointB)) ||
			   (PointA.Equals(Other.PointB) && PointB.Equals(Other.PointA));
	}
};

FORCEINLINE uint32 GetTypeHash(const FTriangleEdge& Edge)
{
	return HashCombine(GetTypeHash(Edge.PointA), GetTypeHash(Edge.PointB));
}
