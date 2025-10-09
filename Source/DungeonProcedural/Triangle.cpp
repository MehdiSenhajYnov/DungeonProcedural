// Fill out your copyright notice in the Description page of Project Settings.
#include "DungeonProcedural/Triangle.h"

#include "ViewportInteractionTypes.h"

FTriangle::FTriangle(FVector pointA, FVector pointB, FVector pointC)
{
	PointA = pointA;
	PointB = pointB;
	PointC = pointC;
}

FVector FTriangle::CenterCircle()
{
	FVector Center;
	FVector Bprime = (PointA+PointB)/2;
	FVector Aprime = (PointA+PointC)/2;

	FVector ABdirection = PointB - PointA;
	FLine PerpAB;
	PerpAB.Direction = FVector(-ABdirection.Y,ABdirection.X,0); 
	PerpAB.Point = Bprime;

	FVector ACdirection = PointA - PointC;
	FLine PerpAC;
	PerpAC.Direction = FVector(-ACdirection.Y,ACdirection.X,0); 
	PerpAC.Point = Aprime;

	FVector M;
	bool result = PerpAB.Intersection(PerpAC,M);
	if (result)
	{
		return FVector();
	}
	return M;
}

float FTriangle::GetArea()
{
	FVector ab = PointB - PointA;
	FVector bc = PointC - PointB;
	FVector ca = PointA - PointC;
	
	float p = (ab.Length() + bc.Length() + ca.Length())/2;

	float S = sqrtf(p*(p-ab.Length())*(p-bc.Length())*(p-ca.Length()));
	return S;
}

float FTriangle::GetRayon()
{
	FVector ab = PointB - PointA;
	FVector bc = PointC - PointB;
	FVector ca = PointA - PointC;

	float S = GetArea();
	float R = ((ab.Length()*bc.Length()*ca.Length())/(2*S))/2;

	return R;
}

void FTriangle::DrawTriangle(const UWorld* InWorld)
{
	DrawDebugLine(InWorld, PointA, PointB,FColor(0,255,0),true,-1,0,50);
	DrawDebugLine(InWorld, PointB, PointC,FColor(0,255,0),true,-1,0,50);
	DrawDebugLine(InWorld, PointC, PointA,FColor(0,255,0),true,-1,0,50);

}

bool FLine::Intersection(FLine LineChecked,FVector& OutCenter)
{
	float det = Direction.X * LineChecked.Direction.Y - Direction.Y * LineChecked.Direction.X;
	if (FMath::IsNearlyZero(det))
	{
		return false;
	}
	FVector AprimeBprime = LineChecked.Point - Point;
	float t2 = (AprimeBprime.X * LineChecked.Direction.Y - AprimeBprime.Y * LineChecked.Direction.X) / det;
	OutCenter = Point*Direction * t2;
	return true;
}
