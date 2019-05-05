#pragma once

//Useful Links 
//https://docs.unrealengine.com/en-us/Programming
//https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Delegates
//https://docs.unrealengine.com/en-us/Programming/UnrealArchitecture/SmartPointerLibrary/WeakPointer
//http://www.cplusplus.com/reference/queue/priority_queue/
//https://wiki.unrealengine.com/UPARAM
//https://stackoverflow.com/questions/28002/regular-cast-vs-static-cast-vs-dynamic-cast
//https://api.unrealengine.com/INT/API/Runtime/Engine/Components/ULineBatchComponent/index.html

//Navigation component.
//Error handling 
//Github Info Page with gifs and flow.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Components/LineBatchComponent.h"
#include "Engine.h"
#include <queue>
#include <vector>
#include "VoxelUtils.generated.h"

#define MAX_VOXEL_NEIGHBOUR 18

#pragma region HelperMethods
static ULineBatchComponent* GetDebugLineBatcherTemp(const UWorld* InWorld, bool bPersistentLines, float LifeTime, bool bDepthIsForeground)
{
	return (InWorld ? (bDepthIsForeground ? InWorld->ForegroundLineBatcher : ((bPersistentLines || (LifeTime > 0.f)) ? InWorld->PersistentLineBatcher : InWorld->LineBatcher)) : NULL);
}


static void DrawDebugVoxelTemp(const UWorld* inWorld, FVector const& center, FVector const& box, FColor const& color, bool bPersistentLines, float lifeTime, uint8 depthPriority, float thickness)
{
	
	ULineBatchComponent* const lineBatchComp = GetDebugLineBatcherTemp(inWorld, bPersistentLines, lifeTime, (depthPriority == SDPG_Foreground));

	if (lineBatchComp != nullptr)
	{
		float LineLifeTime = (lifeTime > 0.f) ? lifeTime : lineBatchComp->DefaultLifeTime;

		lineBatchComp->DrawLine(center + FVector(box.X, box.Y, box.Z), center + FVector(box.X, -box.Y, box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(box.X, -box.Y, box.Z), center + FVector(-box.X, -box.Y, box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(-box.X, -box.Y, box.Z), center + FVector(-box.X, box.Y, box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(-box.X, box.Y, box.Z), center + FVector(box.X, box.Y, box.Z), color, depthPriority, thickness, LineLifeTime);

		lineBatchComp->DrawLine(center + FVector(box.X, box.Y, -box.Z), center + FVector(box.X, -box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(box.X, -box.Y, -box.Z), center + FVector(-box.X, -box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(-box.X, -box.Y, -box.Z), center + FVector(-box.X, box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(-box.X, box.Y, -box.Z), center + FVector(box.X, box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);

		lineBatchComp->DrawLine(center + FVector(box.X, box.Y, box.Z), center + FVector(box.X, box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(box.X, -box.Y, box.Z), center + FVector(box.X, -box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(-box.X, -box.Y, box.Z), center + FVector(-box.X, -box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
		lineBatchComp->DrawLine(center + FVector(-box.X, box.Y, box.Z), center + FVector(-box.X, box.Y, -box.Z), color, depthPriority, thickness, LineLifeTime);
	}

}

//HelperMethods
#pragma endregion

#pragma region Struct

//struct to represent and hold voxel data in 3d space. 
USTRUCT()
struct FVoxel
{
	GENERATED_USTRUCT_BODY()

	FVoxel() {}

	/* Position of voxel in 3d space. */
	FVector position;

	/* Index of voxel in voxel array that makes up the 3d navigation. */
	int32 xIndex, yIndex, zIndex;

	/* True, if the voxel is blocked by a collision mesh. */
	bool bIsBlocked = false;

	/* True, if the voxel is next right next to a blocked volume. ie. it's very close to a surface. */
	bool bIsSurfaceVoxel = false;

	/* Friend function to test whether two voxels have same indexes. */
	friend bool operator== (const FVoxel& v1, const FVoxel& v2)
	{
		return v1.xIndex == v2.xIndex && v1.yIndex == v2.yIndex && v1.zIndex == v2.zIndex;
	}
};

/*
 * Next step, we need to create 3 dimensnal data structure that can hold all the voxels in a specified 3d volume.
 */

 /* Struct to hold all the voxels in z axis. */
USTRUCT()
struct FVoxelY
{
	GENERATED_USTRUCT_BODY()

	FVoxelY() {}

	UPROPERTY()
	TArray<FVoxel> z;

	void Add(FVoxel voxel) { z.Add(voxel); }
};

/* Struct to hold all the voxels in y axis (2d planar). */
USTRUCT()
struct FVoxelX
{
	GENERATED_USTRUCT_BODY()

	FVoxelX() {}

	UPROPERTY()
	TArray<FVoxelY> y;

	void Add(FVoxelY voxel) { y.Add(voxel); }
};

/* Struct to hold all the voxels in x axis (voxel in all three directions). */
USTRUCT()
struct FVoxelXYZ
{
	GENERATED_USTRUCT_BODY()

	FVoxelXYZ() {}

	UPROPERTY()
	TArray<FVoxelX> x;

	void Add(FVoxelX voxel) { x.Add(voxel); }

	/* Function to delete all the voxel. */
	void Empty()
	{
		for (FVoxelX _x : x)
		{
			for (FVoxelY _y : _x.y)
				_y.z.Empty();

			_x.y.Empty();
		}
		x.Empty();
	}
};


//Priority queue to store pathfinding querys' voxels with its distance (and or heuristics).
template<typename T>
struct PriorityQueue
{
	//A user-provided Compare can be supplied to change the ordering, e.g. using std::greater<T> would cause the smallest element to appear as the top(). 
	std::priority_queue<std::pair< int32, T>, std::vector<std::pair<int32, T>>, std::greater< std::pair<int32, T>>> p_queue;

	inline void push(int32 val, T queueElement)
	{
		p_queue.emplace(val, queueElement);
	}

	inline bool empty() { return p_queue.empty(); }

	inline T pop()
	{
		if (!p_queue.size())
			return nullptr;

		T item = p_queue.top().second;
		p_queue.pop();
		return item;
	}
};

UENUM()
enum class EVoxelNavigationStatus : uint8
{
	NotStarted,
	InProgress,
	Failed,
	Succeded,
	TimeOut
};

USTRUCT(BlueprintType)
struct FVoxelPathFindingData
{
	GENERATED_USTRUCT_BODY()

	FVoxelPathFindingData() {}

	FVoxelPathFindingData(AActor* _actor, FVector _origin, FVector _destination, FVoxel* _originVoxel, FVoxel* _destinationVoxel, bool _bUseSurfaceVoxel)
		: actor(_actor), origin(_origin), destination(_destination), originVoxel(_originVoxel), destinationVoxel(_destinationVoxel), bUseSurfaceVoxels(_bUseSurfaceVoxel)
	{}

	/*Input Params*/

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> actor;

	UPROPERTY(BlueprintReadOnly)
	FVector origin;

	UPROPERTY(BlueprintReadOnly)
	FVector destination;

	FVoxel* originVoxel;

	FVoxel* destinationVoxel;

	bool  bUseSurfaceVoxels;

	/*---Input Params---*/

	/*Output Data*/

	PriorityQueue<FVoxel*> frontier;
	TMap<FVoxel*, uint32> weightMap;

	TArray<FVoxel*> voxelPath;
	TArray<FVector> vectorPath;

	TMap<FVoxel*, FVoxel*> pathMap;

	float timeTaken = 0.0f;

	int32 iterationCount = 0;

	UPROPERTY(BlueprintReadOnly)
	EVoxelNavigationStatus status = EVoxelNavigationStatus::NotStarted;

	bool bFoundPath = false;

	/*Output Data*/
};


//Struct
#pragma endregion 