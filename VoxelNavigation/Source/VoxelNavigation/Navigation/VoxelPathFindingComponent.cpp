// Fill out your copyright notice in the Description page of Project Settings.


#include "VoxelPathFindingComponent.h"
#include "VoxelNavigator.h"

// Sets default values for this component's properties
UVoxelPathFindingComponent::UVoxelPathFindingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UVoxelPathFindingComponent::BeginPlay()
{
	Super::BeginPlay();

	pathfindingTasks.Reserve(10);
}


// Called every frame
void UVoxelPathFindingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	deltaTime += DeltaTime;
	//Temporarily slowing down tick to see the pathfinding steps.
	if (deltaTime >= maxTimeBetweenTick)
	{
		PathfindingUpdate(DeltaTime);
		deltaTime = 0.0f;
	}
}

void UVoxelPathFindingComponent::DoRandomTest(FVoxelNavigationHandler resultHandler, bool DoSurfaceNavigation)
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
	while (!found1 && counter < 200 )
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

	FVoxelPathFindingData data = FVoxelPathFindingData(nullptr, start->position, end->position, start, end, DoSurfaceNavigation);

	//FVoxelPathFindingData data1 = FVoxelPathFindingData(nullptr, end->position, start->position, end, start, DoSurfaceNavigation);

	pathfindingTasks.Add(FVoxelPathFindingTask(resultHandler, data));
	//pathfindingTasks.Add(FVoxelPathFindingTask(resultHandler, data1));

}

void UVoxelPathFindingComponent::PathfindingUpdate(float delta)
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
		auto& data = task.data;

		if (data.bFoundPath)
		{
			data.status = EVoxelNavigationStatus::Succeded;
			if (bDebugDraw)
			{
				for (int i = 0; i < data.vectorPath.Num(); ++i)
				{
					if(data.vectorPath.IsValidIndex(i+1))
						DrawDebugLine(this->GetWorld(), data.vectorPath[i], data.vectorPath[i + 1], FColor::Purple, false, 20, 0, 8);
					
				}
			}
			pathfindingTasks.RemoveAtSwap(i);
			//task.resultHandler.Execute(task.data);
			task.BroadcastResult();
		}
		else
		{
			int32 iteration = 1;

			do
			{
				FindPath(data);
				iteration++;
			} while (!data.bFoundPath && iteration <= iterationPerTask);

			data.timeTaken += deltaTime;
		}
	}
}

void UVoxelPathFindingComponent::FindPath(FVoxelPathFindingData& data)
{
	AVoxelNavigator* navigator = Cast<AVoxelNavigator>(GetOwner());

	if (!navigator) return;

	if (!data.frontier.empty())
	{
		auto voxel = data.frontier.pop();

		if (bDebugDraw)
		{
			DrawDebugVoxelTemp(GetWorld(), voxel->position, FVector(navigator->voxelSize / 2, navigator->voxelSize / 2, navigator->voxelSize / 2) * 0.95f, FColor::Red, true, 10.0f, 0, 6.0);
		}

		if (IsPathfindingFinished(voxel,data))
		{
			data.bFoundPath = true;
			FindTrasverablePath(data);
			return;
		}

		TArray<FVoxel*> neighbours;
		neighbours.Reserve(18);

		navigator->GetNeighbourVoxels(*voxel, neighbours);

		for (int32 i = 0; i < neighbours.Num(); ++i)
		{
			FVoxel* item = neighbours[i];

			if (item->bIsBlocked)
				continue;

			if (data.bUseSurfaceVoxels && !item->bIsSurfaceVoxel) continue;

			uint32 weight = *data.weightMap.Find(voxel);
			uint32* neighbourWeight = data.weightMap.Find(item);

			if (bDebugDraw)
			{
				DrawDebugVoxelTemp(GetWorld(), item->position, FVector(navigator->voxelSize / 2, navigator->voxelSize / 2, navigator->voxelSize / 2) * 0.95f, FColor::White, true, 10.0f, 0, 3.0);
			}

			if (data.weightMap.Find(item) == nullptr || (weight + navigator->voxelSize) < *data.weightMap.Find(item))
			{
				data.weightMap.Add(item, weight + navigator->voxelSize);

				float h = FVector::Dist(item->position, data.destination);
				uint32 finalWeight = weight + navigator->voxelSize + h;

				DrawDebugString(GetWorld(), item->position, FString::SanitizeFloat(finalWeight), 0, FColor::White, 5.0, false, 3.0);

				data.frontier.push(finalWeight, item);

				data.pathMap.Add(item, voxel);
			}

		}
	}

	data.iterationCount++;
}

bool UVoxelPathFindingComponent::IsPathfindingFinished(FVoxel* voxel, FVoxelPathFindingData& data)
{
	return (voxel == data.destinationVoxel);
}

void UVoxelPathFindingComponent::FindTrasverablePath(FVoxelPathFindingData& data)
{
	if (data.originVoxel == data.destinationVoxel)
	{
		data.voxelPath.Add(data.originVoxel);
		data.voxelPath.Add(data.originVoxel);

		data.vectorPath.Add(data.origin);
		data.vectorPath.Add(data.origin);

		return;
	}

	data.voxelPath.Insert(data.destinationVoxel, 0);
	data.vectorPath.Insert(data.destination, 0);

	auto next = data.pathMap.Find(data.destinationVoxel);

	bool bStartFound = false;
	while (next)
	{
		if(data.voxelPath.Contains(*next))
			break;

		data.voxelPath.Insert(*next, 0);
		data.vectorPath.Insert((*next)->position, 0);
		if (*next == data.originVoxel)
		{
			bStartFound = true;
			break;
		}
		next = data.pathMap.Find(*next);
	}
}

