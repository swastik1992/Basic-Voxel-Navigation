// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelNavigator.h"

// Sets default values
AVoxelNavigator::AVoxelNavigator()
{
	PrimaryActorTick.bCanEverTick = false;

	navigationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("nav volume"));
	navigationVolume->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	navigationVolume->SetVisibility(true);
	navigationVolume->SetHiddenInGame(false);
	navigationVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	voxelCollisionQueryParams = FCollisionQueryParams(FName("collisionQuery"), false);
	voxelCollisionQueryParams.AddIgnoredActors(actorsToIgnoreForCollision);

	collisionChannels.Add(ECC_WorldStatic);
	collisionChannels.Add(ECC_WorldDynamic);
	voxelSize = 100;
	bShouldSurfaceNavigate = false;
}

// Called when the game starts or when spawned
void AVoxelNavigator::BeginPlay()
{
	Super::BeginPlay();

	gridSizeX = (navigationVolume->GetScaledBoxExtent().X / voxelSize) * 2;
	gridSizeY = (navigationVolume->GetScaledBoxExtent().Y / voxelSize) * 2;
	gridSizeZ = (navigationVolume->GetScaledBoxExtent().Z / voxelSize) * 2;

	for (auto collisionChannel : collisionChannels)
		voxelCollisionObjectQueryParams.AddObjectTypesToQuery(collisionChannel);

	GenerateNavigationVoxels();

	if(bShouldSurfaceNavigate)
	CheckForSurfaceVoxel();
	
}


void AVoxelNavigator::GenerateNavigationVoxels()
{
	navVoxels.x.Reserve(gridSizeX);

	for (int32 i = 0; i < gridSizeX; i++)
	{
		FVoxelX xVoxels;
		xVoxels.y.Reserve(gridSizeY);

		for (int32 j = 0; j < gridSizeY; j++)
		{
			FVoxelY yVoxels;
			yVoxels.z.Reserve(gridSizeZ);

			for (int32 k = 0; k < gridSizeZ; k++)
			{
				FVoxel voxel;

				voxel.position = FVector(i * voxelSize + (this->GetActorLocation().X - navigationVolume->GetScaledBoxExtent().X) + (voxelSize / 2),
					j * voxelSize + (this->GetActorLocation().Y - navigationVolume->GetScaledBoxExtent().Y) + (voxelSize / 2),
					k * voxelSize + (this->GetActorLocation().Z - navigationVolume->GetScaledBoxExtent().Z) + (voxelSize / 2));
				voxel.xIndex = i;
				voxel.yIndex = j;
				voxel.zIndex = k;

				CheckForCollision(voxel);

				yVoxels.Add(voxel);
			}

			xVoxels.Add(yVoxels);
		}

		navVoxels.Add(xVoxels);
	}
}

void AVoxelNavigator::CheckForCollision(FVoxel & voxel)
{
	TArray<FOverlapResult> overlapResults;
	const bool result = GetWorld()->OverlapMultiByObjectType(overlapResults, voxel.position, FQuat::Identity, voxelCollisionObjectQueryParams,
		FCollisionShape::MakeBox(FVector(voxelSize / 2, voxelSize / 2, voxelSize / 2)), voxelCollisionQueryParams);

	voxel.bIsBlocked = result;
}

void AVoxelNavigator::CheckForSurfaceVoxel()
{
	TArray<FVoxel*> neighbors;
	neighbors.Reserve(MAX_VOXEL_NEIGHBOUR);

	for (FVoxelX _x : navVoxels.x)
	{
		FColor color = FColor::MakeRandomColor();
		for (FVoxelY _y : _x.y)
		{
			for (FVoxel _z : _y.z)
			{
				if (_z.bIsBlocked)
				{
					GetNeighbourVoxels(_z, neighbors);
				}
			}
		}
	}

	for (auto& voxel : neighbors)
	{
		if (!voxel->bIsBlocked && !voxel->bIsSurfaceVoxel)
		{
			voxel->bIsSurfaceVoxel = true;
		}
	}
}

void AVoxelNavigator::GetNeighbourVoxels(const FVoxel& voxel, TArray<FVoxel*>& neighbours)
{
	// 6 directional voxel's indexes.
	int xCoords[6] = { 0, 0, 1,-1, 0, 0 };
	int zCoords[6] = { 1,-1, 0, 0, 0, 0 };
	int yCoords[6] = { 0, 0, 0, 0, 1,-1 };

	//first 6 neighbours
	for (int32 i = 0; i < 6; i++)
	{
		int32 _x = voxel.xIndex + xCoords[i];
		int32 _y = voxel.yIndex + yCoords[i];
		int32 _z = voxel.zIndex + zCoords[i];

		if (IsValidVoxel(_x, _y, _z))
			neighbours.Add(&navVoxels.x[_x].y[_y].z[_z]);
	}

	bool xA1 = IsValidVoxel(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex);
	bool xS1 = IsValidVoxel(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex);
	bool yA1 = IsValidVoxel(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex);
	bool yS1 = IsValidVoxel(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex);
	bool zA1 = IsValidVoxel(voxel.xIndex, voxel.yIndex, voxel.zIndex + 1);
	bool zS1 = IsValidVoxel(voxel.xIndex, voxel.yIndex, voxel.zIndex - 1);

	//1
	if (xA1 && zA1 && !GetVoxelFast(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex + 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex + 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex + 1].y[voxel.yIndex].z[voxel.zIndex + 1]);
	}

	//2
	if (xS1 && zA1 && !GetVoxelFast(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex + 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex + 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex - 1].y[voxel.yIndex].z[voxel.zIndex + 1]);
	}

	//3
	if (xA1 && zS1 && !GetVoxelFast(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex - 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex - 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex + 1].y[voxel.yIndex].z[voxel.zIndex - 1]);
	}

	//4
	if (xS1 && zS1 && !GetVoxelFast(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex - 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex - 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex - 1].y[voxel.yIndex].z[voxel.zIndex - 1]);
	}

	//5
	if (yA1 && zA1 && !GetVoxelFast(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex + 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex + 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex].y[voxel.yIndex + 1].z[voxel.zIndex + 1]);
	}

	//6
	if (yS1 && zA1 && !GetVoxelFast(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex + 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex + 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex].y[voxel.yIndex - 1].z[voxel.zIndex + 1]);
	}

	//7
	if (yA1 && zS1 && !GetVoxelFast(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex - 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex - 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex].y[voxel.yIndex + 1].z[voxel.zIndex - 1]);
	}

	//8
	if (yS1 && zS1 && !GetVoxelFast(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex, voxel.zIndex - 1)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex - 1))
			neighbours.Add(&navVoxels.x[voxel.xIndex].y[voxel.yIndex - 1].z[voxel.zIndex - 1]);
	}

	//9
	if (xA1 && yA1 && !GetVoxelFast(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex + 1, voxel.yIndex + 1, voxel.zIndex))
			neighbours.Add(&navVoxels.x[voxel.xIndex + 1].y[voxel.yIndex + 1].z[voxel.zIndex]);
	}

	//10
	if (xS1 && yA1 && !GetVoxelFast(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex + 1, voxel.zIndex)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex - 1, voxel.yIndex + 1, voxel.zIndex))
			neighbours.Add(&navVoxels.x[voxel.xIndex - 1].y[voxel.yIndex + 1].z[voxel.zIndex]);
	}

	//11
	if (xA1 && yS1 && !GetVoxelFast(voxel.xIndex + 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex + 1, voxel.yIndex - 1, voxel.zIndex))
			neighbours.Add(&navVoxels.x[voxel.xIndex + 1].y[voxel.yIndex - 1].z[voxel.zIndex]);
	}

	//12
	if (xS1 && yS1 && !GetVoxelFast(voxel.xIndex - 1, voxel.yIndex, voxel.zIndex)->bIsBlocked &&
		!GetVoxelFast(voxel.xIndex, voxel.yIndex - 1, voxel.zIndex)->bIsBlocked)
	{
		if (IsValidVoxel(voxel.xIndex - 1, voxel.yIndex - 1, voxel.zIndex))
			neighbours.Add(&navVoxels.x[voxel.xIndex - 1].y[voxel.yIndex - 1].z[voxel.zIndex]);
	}
}

void AVoxelNavigator::DebugVoxels(float time)
{
	for (FVoxelX _x : navVoxels.x)
	{
		FColor color = FColor::MakeRandomColor();
		for (FVoxelY _y : _x.y)
		{
			for (FVoxel _z : _y.z)
			{
				//if (_z.bIsSurfaceVoxel)
				//	DrawDebugVoxelTemp(GetWorld(), _z.position, FVector(voxelSize / 2, voxelSize / 2, voxelSize / 2) * 0.95f, FColor::White, false, time, 0, 1.0);
				if(_z.bIsBlocked)
					DrawDebugVoxelTemp(GetWorld(), _z.position, FVector(voxelSize / 2, voxelSize / 2, voxelSize / 2) * 0.95f, FColor::Black, false, time, 0, 10.0);
				else
					DrawDebugVoxelTemp(GetWorld(), _z.position, FVector(voxelSize / 2, voxelSize / 2, voxelSize / 2) * 0.95f, color, false, time, 0, 2.0);
			}
		}
	}
}

void AVoxelNavigator::RefreshVoxels()
{
	navVoxels.Empty();

	gridSizeX = (navigationVolume->GetScaledBoxExtent().X / voxelSize) * 2;
	gridSizeY = (navigationVolume->GetScaledBoxExtent().Y / voxelSize) * 2;
	gridSizeZ = (navigationVolume->GetScaledBoxExtent().Z / voxelSize) * 2;

	GenerateNavigationVoxels();

	if (bShouldSurfaceNavigate)
		CheckForSurfaceVoxel();
}
