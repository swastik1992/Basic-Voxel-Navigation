// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelUtils.h"
#include "Components/BoxComponent.h"
#include "VoxelNavigator.generated.h"

UCLASS()
class VOXELNAVIGATION_API AVoxelNavigator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVoxelNavigator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	
	UFUNCTION(BlueprintCallable)
	void DebugVoxels(float time = 10.0f);

	
	void GenerateNavigationVoxels();

	void CheckForCollision(FVoxel& voxel);

	void CheckForSurfaceVoxel();

	/*
	//	Function to get all the neighbouring voxels in 3d. Usually 18 voxels. 
	*/
	void GetNeighbourVoxels(const FVoxel& voxel, TArray<FVoxel*>& neighbours);
	
	/*
	//	Function to check if a voxel of input indices exists in nav voxel array. 
	*/
	FORCEINLINE bool IsValidVoxel(int x, int y, int z)
	{
		return navVoxels.x.IsValidIndex(x) && navVoxels.x[x].y.IsValidIndex(y) && navVoxels.x[x].y[y].z.IsValidIndex(z);
	}

	/*
	//	Function to get voxel from nav voxel array without checking it's existence.
	*/
	FORCEINLINE FVoxel* GetVoxelFast(int x, int y, int z)
	{
		return &navVoxels.x[x].y[y].z[z];
	}

	//Size of one voxel. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float voxelSize;

	//Collision channel to use for object types collision query will be inserted to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TEnumAsByte<ECollisionChannel>> collisionChannels;

	//List of particular actors to ignore. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> actorsToIgnoreForCollision;

	UPROPERTY(EditAnywhere)
	UBoxComponent* navigationVolume;

	//To define surface navigation voxels. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldSurfaceNavigate;

private:

	UPROPERTY()
	FVoxelXYZ navVoxels;

	int32 gridSizeX, gridSizeY, gridSizeZ;
	FCollisionQueryParams voxelCollisionQueryParams;
	FCollisionObjectQueryParams voxelCollisionObjectQueryParams;
};
