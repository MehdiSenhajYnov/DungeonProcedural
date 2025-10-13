// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/ConfigRoomDataAsset.h"
#include "DungeonProcedural/Triangle.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManager.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEONPROCEDURAL_API URoomManager : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<ARoomParent*> SpawnedActors;

	UPROPERTY()
	TArray<AActor*> OtherActorsToClear;
	
	UFUNCTION(BlueprintCallable)
	void GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes);

	UFUNCTION(BlueprintCallable)
	void MegaTriangle(TSubclassOf<ARoomParent> Room);

	UFUNCTION(BlueprintCallable)
	void Triangulation(TSubclassOf<ARoomParent> RoomP);

	UFUNCTION(BlueprintCallable)
	void CreatePath(TSubclassOf<ARoomParent> RoomP);

	UFUNCTION(BlueprintCallable)
	void EvolvePath();	

	UFUNCTION(BlueprintCallable)
	void SpawnConnectionModules(TSubclassOf<AActor> CorridorBP);
	
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
	
	

	// Fonction principale à appeler
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
	
	// Étapes internes
	void StepByStepTriangulation(TSubclassOf<ARoomParent> RoomP);
	void StepByStepPrim(TSubclassOf<ARoomParent> RoomP);
	void StepByStepEvolvePath();
	void StepByStepClear(TSubclassOf<ARoomParent> RoomS);
	void StepByStepCorridors(TSubclassOf<ARoomParent> RoomC);
	void ResetStepByStep();
	void RedrawStableState();

private:
	bool CheckOverlapping(const UBoxComponent* BoxA, const UBoxComponent* BoxB);

	// Pour la triangulation progressive
	int CurrentStep = 0;
	TArray<AActor*> RoomPrincipallist;
	int CurrentPointIndex = 0;
	int CurrentLittleStep = 0;
	int32 StepRunId = 0;      // id de la génération en cours
	int32 ActiveRunId = -1;   // ce qu'on a dessiné
	bool bMSTInitialized = false;
	bool bPathsEvolved = false;

	// état auto
	void AutoTick(); // une "avance" de plus dans la démo
	
	bool bAutoDemo = false;
	float AutoDelay = 0.6f;
	FTimerHandle AutoDemoTimer;

	TSubclassOf<ARoomParent> AutoRoomP;
	TSubclassOf<ARoomParent> AutoRoomS;
	TSubclassOf<ARoomParent> AutoRoomC;
	
	// Stocker les positions du méga-triangle pour pouvoir les supprimer à la fin
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

