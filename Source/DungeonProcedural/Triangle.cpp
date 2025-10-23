// Fill out your copyright notice in the Description page of Project Settings.
#include "DungeonProcedural/Triangle.h"

#include "MathUtil.h"
#include "ViewportInteractionTypes.h"

FTriangle::FTriangle(FVector pointA, FVector pointB, FVector pointC)
{
	PointA = pointA;
	PointB = pointB;
	PointC = pointC;
	points.Add(PointA);
	points.Add(PointB);
	points.Add(PointC);
}

TArray<FVector> FTriangle::GetAllPoints() const
{
	TArray<FVector> AllPoints;
	AllPoints.Add(PointA);
	AllPoints.Add(PointB);
	AllPoints.Add(PointC);
	return AllPoints;
}

TArray<FTriangleEdge> FTriangle::GetEdges() const
{
	TArray<FTriangleEdge> Edges;
	Edges.Add(FTriangleEdge(PointA, PointB));
	Edges.Add(FTriangleEdge(PointB, PointC));
	Edges.Add(FTriangleEdge(PointC, PointA));
	return Edges;
}

bool FTriangle::CenterCircle(FVector& outCenter) const
{
	// Calculate determinant for circumcenter formula
	double D = 2 * (PointA.X * (PointB.Y - PointC.Y) + PointB.X * (PointC.Y - PointA.Y) + PointC.X * (PointA.Y - PointB.Y));
	if (FMath::IsNearlyZero(D))
	{
		UE_LOG(LogTemp, Warning, TEXT("Points are collinear!"))
		return false;
	}
    
	// Calculate circumcenter coordinates using determinant formula
	double ux = ((PointA.X*PointA.X + PointA.Y*PointA.Y) * (PointB.Y - PointC.Y) + 
				 (PointB.X*PointB.X + PointB.Y*PointB.Y) * (PointC.Y - PointA.Y) + 
				 (PointC.X*PointC.X + PointC.Y*PointC.Y) * (PointA.Y - PointB.Y)) / D;
    
	double uy = ((PointA.X*PointA.X + PointA.Y*PointA.Y) * (PointC.X - PointB.X) + 
				 (PointB.X*PointB.X + PointB.Y*PointB.Y) * (PointA.X - PointC.X) + 
				 (PointC.X*PointC.X + PointC.Y*PointC.Y) * (PointB.X - PointA.X)) / D;
    
	outCenter = FVector(ux, uy,0);
	return true;
}

float FTriangle::GetArea() const
{
	// Calculate triangle area using Heron's formula
	FVector ab = PointB - PointA;
	FVector bc = PointC - PointB;
	FVector ca = PointA - PointC;
	
	// Semi-perimeter
	float p = (ab.Length() + bc.Length() + ca.Length())/2;

	// Heron's formula: S = sqrt(p*(p-a)*(p-b)*(p-c))
	float S = sqrtf(p*(p-ab.Length())*(p-bc.Length())*(p-ca.Length()));
	return S;
}

float FTriangle::GetRayon() const
{
	// Calculate circumradius for Delaunay triangulation point-in-circle tests
	FVector ab = PointB - PointA;
	FVector bc = PointC - PointB;
	FVector ca = PointA - PointC;

	float S = GetArea();
	// Circumradius formula: R = (a*b*c)/(4*Area)
	float R = ((ab.Length()*bc.Length()*ca.Length())/(2*S))/2;

	return R;
}

void FTriangle::DrawTriangle(const UWorld* InWorld, FColor ColorToUse) const
{
	DrawDebugLine(InWorld, PointA, PointB, ColorToUse,true,-1,0,50);
	DrawDebugLine(InWorld, PointB, PointC, ColorToUse,true,-1,0,50);
	DrawDebugLine(InWorld, PointC, PointA, ColorToUse,true,-1,0,50);

}

bool FLine::Intersection(FLine LineChecked,FVector& OutCenter)
{
	// Calculate line intersection using determinant method
	float det = Direction.X * LineChecked.Direction.Y - Direction.Y * LineChecked.Direction.X;
	if (FMath::IsNearlyZero(det))
	{
		// Lines are parallel
		return false;
	}
	FVector AprimeBprime = LineChecked.Point - Point;
	float t2 = (AprimeBprime.X * LineChecked.Direction.Y - AprimeBprime.Y * LineChecked.Direction.X) / det;
	OutCenter = Point*Direction * t2;
	return true;
}

float FTriangleEdge::GetLength() const
{
	return (PointB - PointA).Length();
}

void FTriangleEdge::DrawEdge(const UWorld* InWorld, FColor ColorToUse) const
{
	DrawDebugLine(InWorld, PointA, PointB, ColorToUse,true,-1,0,50);
}
