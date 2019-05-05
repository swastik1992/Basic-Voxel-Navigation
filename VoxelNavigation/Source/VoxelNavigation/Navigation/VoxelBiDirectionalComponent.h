// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/VoxelPathFindingComponent.h"
#include "VoxelBiDirectionalComponent.generated.h"


USTRUCT()
struct FVoxelBiDirectionPathFindingTask
{
	GENERATED_USTRUCT_BODY()

	FVoxelBiDirectionPathFindingTask() {}

	FVoxelBiDirectionPathFindingTask(FVoxelNavigationHandler _resultHandler, FVoxelPathFindingData _dataForward, FVoxelPathFindingData _dataBackward)
		:resultHandler(_resultHandler), dataForward(_dataForward), dataBackward(_dataBackward)
	{
		dataForward.status = EVoxelNavigationStatus::InProgress;
		dataForward.frontier.push(0, dataForward.originVoxel);
		dataForward.weightMap.Add(dataForward .originVoxel, 0);

		dataBackward.status = EVoxelNavigationStatus::InProgress;
		dataBackward.frontier.push(0, dataBackward.originVoxel);
		dataBackward.weightMap.Add(dataBackward.originVoxel, 0);
	}

	void BroadcastResult()
	{
		resultHandler.ExecuteIfBound(data);
	}

	UPROPERTY()
	FVoxelNavigationHandler resultHandler;

	FVoxelPathFindingData dataForward;
	FVoxelPathFindingData dataBackward;

	FVoxelPathFindingData data;
};


/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VOXELNAVIGATION_API UVoxelBiDirectionalComponent : public UVoxelPathFindingComponent
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	virtual void DoRandomTest(FVoxelNavigationHandler resultHandler, bool DoSurfaceNavigation) override;


	virtual void PathfindingUpdate(float delta) override;


	virtual bool IsPathfindingFinished(FVoxel* voxel, FVoxelPathFindingData& data) override;


	virtual void FindTrasverablePath(FVoxelPathFindingData& data) override;

private:

	void FindBiDirectionPath(FVoxelPathFindingData& dataForward, FVoxelPathFindingData& dataBackward, FVoxelPathFindingData& data);
	void FindBiDirectionTrasverablePath(FVoxelPathFindingData& dataForward, FVoxelPathFindingData& dataBackward, FVoxelPathFindingData& datan, FVoxel* sharedVoxel);
	void BoradcastResult(int32 taskId, FVoxelPathFindingData& data);
	TArray<FVoxelBiDirectionPathFindingTask> pathfindingTasks;

};
