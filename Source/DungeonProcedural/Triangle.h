// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Triangle.generated.h"

// Triangle structure for Delaunay triangulation
// Contains three points and geometric calculation methods
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

	// Returns all three vertices of the triangle
	TArray<FVector> GetAllPoints() const;

	// Returns the three edges of the triangle
	TArray<FTriangleEdge> GetEdges() const;

	// Calculates circumcenter for Delaunay triangulation
	bool CenterCircle(FVector& outCenter) const;

	// Calculates triangle area using Heron's formula
	float GetArea() const;

	// Calculates circumradius for point-in-circle tests
	float GetRayon() const;

	// Debug visualization in editor
	void DrawTriangle(const UWorld* InWorld, FColor ColorToUse = FColor(0,255,0)) const;
private:
	TArray<FVector> points;
};

// Line structure for geometric calculations
USTRUCT(BlueprintType)
struct FLine
{
	GENERATED_BODY()
	UPROPERTY()
	FVector Direction;

	UPROPERTY()
	FVector Point;

	// Calculates intersection point between two lines
	bool Intersection(FLine LineChecked,FVector& OutCenter);
};

// Edge structure representing a connection between two points
// Used for MST creation and corridor generation
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

	// Returns the length of the edge
	float GetLength() const;

	// Debug visualization in editor
	void DrawEdge(const UWorld* InWorld, FColor ColorToUse = FColor(0,255,0)) const;

	// Checks if edge is horizontal or vertical (for L-shaped corridor optimization)
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
	
	// Equality operator - edges are equal regardless of point order
	bool operator==(const FTriangleEdge& Other) const
	{
		return (PointA.Equals(Other.PointA) && PointB.Equals(Other.PointB)) ||
			   (PointA.Equals(Other.PointB) && PointB.Equals(Other.PointA));
	}
};

// Hash function for using FTriangleEdge in TSet/TMap containers
FORCEINLINE uint32 GetTypeHash(const FTriangleEdge& Edge)
{
	return HashCombine(GetTypeHash(Edge.PointA), GetTypeHash(Edge.PointB));
}
