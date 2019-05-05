// Fill out your copyright notice in the Description page of Project Settings.


#include "VoxelBiDirectionalComponent.h"
#include "VoxelNavigator.h"

void UVoxelBiDirectionalComponent::BeginPlay()
{
	Super::BeginPlay();

	pathfindingTasks.Reserve(20);
}

void UVoxelBiDirectionalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVoxelBiDirectionalComponent::DoRandomTest(FVoxelNavigationHandler resultHandler, bool DoSurfaceNavigation)
{
	AVoxelNavigator* navigator = Cast<AVoxelNavigator>(GetOwner());
	if (!navigator) return;

	FVoxel* start = nullptr;
	FVoxel* end = nullptr;

	bool found = false;
	int counter = 0;
	while (!found && counter < 200)
	{
		int32 randX = FMath::RandRange(0, navigator->navVoxels.x.Num() - 1);
		int32 randY = FMath::RandRange(0, navigator->navVoxels.x[randX].y.Num() - 1);
		int32 randZ = FMath::RandRange(0, navigator->navVoxels.x[randX].y[randY].z.Num() - 1);

		if (navigator->IsValidVoxel(randX, randY, randZ))
			start = &navigator->navVoxels.x[randX].y[randY].z[randZ];

		if (DoSurfaceNavigation && start->bIsSurfaceVoxel) found = true;
		if (!DoSurfaceNavigation && !start->bIsBlocked) found = true;

		counter++;
	}

	if (counter >= 200) return;

	bool found1 = false; counter = 0;
	while (!found1 && counter < 200)
	{
		int32 randX = FMath::RandRange(0, navigator->navVoxels.x.Num() - 1);
		int32 randY = FMath::RandRange(0, navigator->navVoxels.x[randX].y.Num() - 1);
		int32 randZ = FMath::RandRange(0, navigator->navVoxels.x[randX].y[randY].z.Num() - 1);

		if (navigator->IsValidVoxel(randX, randY, randZ))
			end = &navigator->navVoxels.x[randX].y[randY].z[randZ];

		if (DoSurfaceNavigation && end->bIsSurfaceVoxel) found1 = true;
		if (!DoSurfaceNavigation && !end->bIsBlocked) found1 = true;

		counter++;
	}

	if (!start || !end) return;

	DrawDebugSphere(GetWorld(), start->position, 20, 14, FColor::Green, true, 50.0f, 0, 10);

	DrawDebugSphere(GetWorld(), end->position, 20, 14, FColor::Emerald, true, 50.0f, 0, 10);

	FVoxelPathFindingData dataForward = FVoxelPathFindingData(nullptr, start->position, end->position, start, end, DoSurfaceNavigation);

	FVoxelPathFindingData dataBackward = FVoxelPathFindingData(nullptr, end->position, start->position, end, start, DoSurfaceNavigation);

	pathfindingTasks.Add(FVoxelBiDirectionPathFindingTask(resultHandler, dataForward, dataBackward));
}

void UVoxelBiDirectionalComponent::PathfindingUpdate(float delta)
{
	if (!pathfindingTasks.Num()) return;

	int32 taskHandledThisTick = maxIterationPerTick;
	int32 iterationPerTask = 2;

	if (pathfindingTasks.Num() <= maxIterationPerTick)
	{
		taskHandledThisTick = pathfindingTasks.Num();
		iterationPerTask = maxIterationPerTick / pathfindingTasks.Num();
	}

	for (int32 i = taskHandledThisTick - 1; i >= 0; i--)
	{
		auto& task = pathfindingTasks[i];
		auto& dataForward = task.dataForward;
		auto& dataBackward = task.dataBackward;
		auto& data = task.data;

		if (data.bFoundPath || dataForward.bFoundPath || dataBackward.bFoundPath)
		{
			if (dataForward.bFoundPath)
			{
				BoradcastResult(i, dataForward);
			}
			else if (dataBackward.bFoundPath)
			{
				BoradcastResult(i, dataBackward);
			}
			else if (data.bFoundPath)
			{
				BoradcastResult(i, data);
			}
		}
		else
		{
			int32 iteration = 1;

			do
			{
				FindBiDirectionPath(dataForward, dataBackward, data);				
				iteration++;
			} while (!dataForward.bFoundPath && !dataBackward.bFoundPath && iteration <= iterationPerTask);

			dataForward.timeTaken += deltaTime;
			dataBackward.timeTaken += deltaTime;
		}
	}
}

bool UVoxelBiDirectionalComponent::IsPathfindingFinished(FVoxel* voxel, FVoxelPathFindingData& data)
{
	return Super::IsPathfindingFinished(voxel,data);
}

void UVoxelBiDirectionalComponent::FindTrasverablePath(FVoxelPathFindingData& data)
{
	Super::FindTrasverablePath(data);
}

void UVoxelBiDirectionalComponent::FindBiDirectionPath(FVoxelPathFindingData& dataForward, FVoxelPathFindingData& dataBackward, FVoxelPathFindingData& data)
{
	AVoxelNavigator* navigator = Cast<AVoxelNavigator>(GetOwner());

	if (!navigator) return;

	FVoxel* voxelForward = nullptr;
	if (!dataForward.frontier.empty())
	{
		voxelForward = dataForward.frontier.pop();
	}

	FVoxel* voxelBackward = nullptr;
	if (!dataBackward.frontier.empty())
	{
		voxelBackward = dataBackward.frontier.pop();
	}

	if (voxelForward)
	{
		auto v1 = dataBackward.pathMap.Find(voxelForward);
		if (v1)
		{
			data.bFoundPath = true;
			FindBiDirectionTrasverablePath(dataForward, dataBackward, data, *v1);
			return;
		}
	}

	if (voxelBackward)
	{
		auto v2 = dataForward.pathMap.Find(voxelBackward);
		if (v2)
		{
			data.bFoundPath = true;
			FindBiDirectionTrasverablePath(dataForward, dataBackward, data, *v2);
			return;
		}
	}

	//check if the path has met.
	if (voxelForward == voxelBackward)
	{
		data.bFoundPath = true;
		FindBiDirectionTrasverablePath(dataForward, dataBackward, data, voxelForward);
		return;
	}
	else
	{

		if (voxelForward)
		{
			if (bDebugDraw)
			{
				DrawDebugVoxelTemp(GetWorld(), voxelForward->position, FVector(navigator->voxelSize / 2, navigator->voxelSize / 2, navigator->voxelSize / 2) * 0.95f, FColor::Green, true, 10.0f, 0, 6.0);
			}

			if (IsPathfindingFinished(voxelForward ,dataForward))
			{
				dataForward.bFoundPath = true;
				FindTrasverablePath(dataForward);
				return;
			}

			TArray<FVoxel*> neighbours;
			neighbours.Reserve(18);

			navigator->GetNeighbourVoxels(*voxelForward, neighbours);

			for (int32 i = 0; i < neighbours.Num(); ++i)
			{
				FVoxel* item = neighbours[i];

				if (item->bIsBlocked)
					continue;

				if (dataForward.bUseSurfaceVoxels && !item->bIsSurfaceVoxel) continue;

				uint32 weight = *dataForward.weightMap.Find(voxelForward);
				uint32* neighbourWeight = dataForward.weightMap.Find(item);

				if (bDebugDraw)
				{
					DrawDebugVoxelTemp(GetWorld(), item->position, FVector(navigator->voxelSize / 2, navigator->voxelSize / 2, navigator->voxelSize / 2) * 0.95f, FColor::Yellow, true, 10.0f, 0, 3.0);
				}

				if (dataForward.weightMap.Find(item) == nullptr || (weight + navigator->voxelSize) < *dataForward.weightMap.Find(item))
				{
					dataForward.weightMap.Add(item, weight + navigator->voxelSize);
					
					float h = FVector::Dist(item->position, dataForward.destination);

					//if(voxelBackward)
					//	h = FVector::Dist(item->position, voxelBackward->position );
					
					uint32 finalWeight = weight + navigator->voxelSize + h;

					DrawDebugString(GetWorld(), item->position, FString::SanitizeFloat(finalWeight), 0, FColor::White, 5.0, false, 3.0);

					dataForward.frontier.push(finalWeight, item);

					dataForward.pathMap.Add(item, voxelForward);
				}

			}
		}

		if (voxelBackward)
		{
			if (bDebugDraw)
			{
				DrawDebugVoxelTemp(GetWorld(), voxelBackward->position, FVector(navigator->voxelSize / 2, navigator->voxelSize / 2, navigator->voxelSize / 2) * 0.95f, FColor::Red, true, 30.0f, 0, 6.0);
			}

			if (IsPathfindingFinished(voxelBackward, dataBackward))
			{
				dataBackward.bFoundPath = true;
				FindTrasverablePath(dataBackward);
				return;
			}

			TArray<FVoxel*> neighbours;
			neighbours.Reserve(18);

			navigator->GetNeighbourVoxels(*voxelBackward, neighbours);

			for (int32 i = 0; i < neighbours.Num(); ++i)
			{
				FVoxel* item = neighbours[i];

				if (item->bIsBlocked)
					continue;

				if (dataBackward.bUseSurfaceVoxels && !item->bIsSurfaceVoxel) continue;

				uint32 weight = *dataBackward.weightMap.Find(voxelBackward);
				uint32* neighbourWeight = dataBackward.weightMap.Find(item);

				if (bDebugDraw)
				{
					DrawDebugVoxelTemp(GetWorld(), item->position, FVector(navigator->voxelSize / 2, navigator->voxelSize / 2, navigator->voxelSize / 2) * 0.95f, FColor::White, true, 30.0f, 0, 3.0);
				}

				if (dataBackward.weightMap.Find(item) == nullptr || (weight + navigator->voxelSize) < *dataBackward.weightMap.Find(item))
				{
					dataBackward.weightMap.Add(item, weight + navigator->voxelSize);

					float h = FVector::Dist(item->position, dataBackward.destination);

					//if (voxelForward)
					//	h = FVector::Dist(item->position, voxelForward->position);

					uint32 finalWeight = weight + navigator->voxelSize + h;

					DrawDebugString(GetWorld(), item->position, FString::SanitizeFloat(finalWeight), 0, FColor::White, 5.0, false, 3.0);

					dataBackward.frontier.push(finalWeight, item);

					dataBackward.pathMap.Add(item, voxelBackward);
				}

			}
		}

		dataForward.iterationCount++;
		dataBackward.iterationCount++;
	}
}

void UVoxelBiDirectionalComponent::FindBiDirectionTrasverablePath(FVoxelPathFindingData& dataForward, FVoxelPathFindingData& dataBackward, FVoxelPathFindingData& data, FVoxel* sharedVoxel)
{
	//@todo many problem here.

	data.voxelPath.Insert(data.destinationVoxel, 0);
	data.vectorPath.Insert(data.destination, 0);

	//this is from C to A. considering a path from A to B, C is the point where forward and wackward queries meet.
	auto next = dataForward.pathMap.Find(sharedVoxel);
	while (next)
	{
		if (dataForward.voxelPath.Contains(*next))
			break;

		dataForward.voxelPath.Insert(*next, 0);
		dataForward.vectorPath.Insert((*next)->position, 0);
	
		if (*next == dataForward.originVoxel)
		{
			break;
		}
		next = dataForward.pathMap.Find(*next);
	}
	data.voxelPath.Append(dataForward.voxelPath);
	data.vectorPath.Append(dataForward.vectorPath);

	//for backward

	next = dataBackward.pathMap.Find(sharedVoxel);
	while (next)
	{
		if (dataBackward.voxelPath.Contains(*next))
			break;
		
		dataBackward.voxelPath.Add(*next);
		dataBackward.vectorPath.Add((*next)->position);
	
		if (*next == dataBackward.originVoxel)
		{
			break;
		}
		next = dataBackward.pathMap.Find(*next);
	}
	data.voxelPath.Append(dataBackward.voxelPath);
	data.vectorPath.Append(dataBackward.vectorPath);
}

void UVoxelBiDirectionalComponent::BoradcastResult(int32 taskId, FVoxelPathFindingData& data)
{
	auto& task = pathfindingTasks[taskId];

	data.status = EVoxelNavigationStatus::Succeded;
	if (bDebugDraw)
	{
		/*for (int i = 0; i < data.vectorPath.Num(); ++i)
		{
			if (data.vectorPath.IsValidIndex(i + 1))
				DrawDebugLine(this->GetWorld(), data.vectorPath[i], data.vectorPath[i + 1], FColor::Purple, false, 20, 0, 8);

		}*/
	}
	task.resultHandler.ExecuteIfBound(data);
	pathfindingTasks.RemoveAtSwap(taskId);
}
