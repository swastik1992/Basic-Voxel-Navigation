// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VoxelUtils.h"
#include "Components/ActorComponent.h"
#include "VoxelPathFindingComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FVoxelNavigationHandler, const FVoxelPathFindingData&, data);

USTRUCT()
struct FVoxelPathFindingTask
{
	GENERATED_USTRUCT_BODY()

	FVoxelPathFindingTask() {}

	FVoxelPathFindingTask(FVoxelNavigationHandler _resultHandler, FVoxelPathFindingData _data)
		:resultHandler(_resultHandler), data(_data)
	{
		data.status = EVoxelNavigationStatus::InProgress;
		data.frontier.push(0, data.originVoxel);
		data.weightMap.Add(data.originVoxel, 0);
	}

	void BroadcastResult()
	{
		resultHandler.ExecuteIfBound(data);
	}

	UPROPERTY()
	FVoxelNavigationHandler resultHandler;

	FVoxelPathFindingData data;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELNAVIGATION_API UVoxelPathFindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVoxelPathFindingComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 maxIterationPerTick = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float maxTimeBetweenTick = 0.5f;// just for the sake of debugging.

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugDraw = false;// just for the sake of debugging.

	UFUNCTION(BlueprintCallable)
	virtual void DoRandomTest(FVoxelNavigationHandler resultHandler, bool DoSurfaceNavigation);

protected:

	virtual void PathfindingUpdate(float delta);

	virtual void FindPath(FVoxelPathFindingData& data);

	virtual bool IsPathfindingFinished(FVoxel* voxel, FVoxelPathFindingData& data);

	virtual void FindTrasverablePath(FVoxelPathFindingData& data);

	float deltaTime = 0.0f;// just for the sake of debugging.
	
	// 6 directional voxel's indexes.
	int xCoords[6] = { 0, 0, 1,-1, 0, 0 };
	int zCoords[6] = { 1,-1, 0, 0, 0, 0 };
	int yCoords[6] = { 0, 0, 0, 0, 1,-1 };

	bool bSurfacNav = false;
	
private:

	TArray<FVoxelPathFindingTask> pathfindingTasks;
};
