// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/ConfigRoomDataAsset.h"
#include "DungeonProcedural/Triangle.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManager.generated.h"

/**
 * World Subsystem responsible for procedural dungeon generation.
 * Handles triangulation, room spawning, overlap resolution, and path creation.
 */
UCLASS()
class DUNGEONPROCEDURAL_API URoomManager : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// Array of all spawned room actors in the dungeon
	UPROPERTY()
	TArray<ARoomParent*> SpawnedActors;

	// Array of temporary actors (mega-triangle points, corridors) to be cleaned up
	UPROPERTY()
	TArray<AActor*> OtherActorsToClear;
	
	// Main entry point: generates a complete dungeon with specified number and types of rooms
	UFUNCTION(BlueprintCallable)
	void GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes);

	// Creates the super-triangle that encompasses all rooms for Delaunay triangulation
	UFUNCTION(BlueprintCallable)
	void MegaTriangle(TSubclassOf<ARoomParent> Room);

	// Performs Delaunay triangulation on all primary rooms
	UFUNCTION(BlueprintCallable)
	void Triangulation(TSubclassOf<ARoomParent> RoomP);

	// Creates minimum spanning tree (MST) from triangulated rooms for connectivity
	UFUNCTION(BlueprintCallable)
	void CreatePath(TSubclassOf<ARoomParent> RoomP);

	// Converts MST edges into L-shaped corridors (horizontal + vertical segments)
	UFUNCTION(BlueprintCallable)
	void EvolvePath();	

	UFUNCTION(BlueprintCallable)
	void SpawnConnectionModules(TSubclassOf<AActor> CorridorBP);
	
	// Removes secondary rooms that don't intersect with corridor paths
	UFUNCTION(BlueprintCallable)
	void ClearSecondaryRoom(TSubclassOf<ARoomParent> SecondaryRoomType);
	bool IsSegmentIntersectingBox(const FVector& PointA, const FVector& PointB, const FVector& CenterOfBox, FVector Size);

	UPROPERTY()
	TArray<FTriangle> TriangleErased;
	UPROPERTY()
	TArray<FTriangle> LastTrianglesCreated;

	UPROPERTY()
	TArray<FTriangle> AllTriangles;

	UPROPERTY()
	TArray<FTriangleEdge> FirstPath;

	UPROPERTY()
	TArray<FTriangleEdge> EvolvedPath;
	
	UPROPERTY()
	bool TriangulationDone = false;

	// Main function for resolving room overlaps using physics-based separation
	void ResolveRoomOverlaps(TArray<ARoomParent*>& SpawnedActors);


	void DrawAll();
	void ClearDrawAll();

	UFUNCTION(BlueprintCallable)
	void ClearAll();

	template<typename T>
	const T* GetAnyElement(const TSet<T>& Set);

	UFUNCTION(BlueprintCallable)
	void StepByStep(TSubclassOf<ARoomParent> RoomP,TSubclassOf<ARoomParent> RoomS,TSubclassOf<ARoomParent> RoomC);

	UFUNCTION(BlueprintCallable)
	void StartAutoDemo(TSubclassOf<ARoomParent> RoomP,TSubclassOf<ARoomParent> RoomS,TSubclassOf<ARoomParent> RoomC,float StepDelaySeconds);

	UFUNCTION(BlueprintCallable)
	void StopAutoDemo();
	
	// Internal step-by-step visualization functions
	void StepByStepTriangulation(TSubclassOf<ARoomParent> RoomP);
	void StepByStepPrim(TSubclassOf<ARoomParent> RoomP);
	void StepByStepEvolvePath();
	void StepByStepClear(TSubclassOf<ARoomParent> RoomS);
	void StepByStepCorridors(TSubclassOf<ARoomParent> RoomC);
	void ResetStepByStep();
	void RedrawStableState();

private:
	bool CheckOverlapping(const UBoxComponent* BoxA, const UBoxComponent* BoxB);

	// Progressive triangulation state variables
	int CurrentStep = 0;
	TArray<AActor*> RoomPrincipallist;
	int CurrentPointIndex = 0;
	int CurrentLittleStep = 0;
	int32 StepRunId = 0;
	int32 ActiveRunId = -1; 
	bool bMSTInitialized = false;
	bool bPathsEvolved = false;

	// Automatic demo state
	void AutoTick();
	
	bool bAutoDemo = false;
	float AutoDelay = 0.6f;
	FTimerHandle AutoDemoTimer;

	TSubclassOf<ARoomParent> AutoRoomP;
	TSubclassOf<ARoomParent> AutoRoomS;
	TSubclassOf<ARoomParent> AutoRoomC;
	
	// Store mega-triangle positions for cleanup after triangulation
	FVector MegaTrianglePointA;
	FVector MegaTrianglePointB;
	FVector MegaTrianglePointC;
	
	void RemoveSuperTriangles();
	
};

struct ToDrawCircle
{
	ToDrawCircle() = default;

	ToDrawCircle(const FVector& Center, float Radius)
		: Center(Center),
		  radius(Radius)
	{
	}

	FVector Center;
	float radius;
};

