#include "Wave/CWSSpawnPoint.h"

#include "Components/SceneComponent.h"

ACWSSpawnPoint::ACWSSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

FTransform ACWSSpawnPoint::GetSpawnTransform() const
{
	return GetActorTransform();
}

bool ACWSSpawnPoint::CanSpawn() const
{
	return bEnabled && !IsActorBeingDestroyed();
}

ECWSSpawnDirection ACWSSpawnPoint::GetSpawnDirection() const
{
	static const TPair<FName, ECWSSpawnDirection> DirectionTags[] = {
		{TEXT("CWS_Spawn_North"), ECWSSpawnDirection::North},
		{TEXT("CWS_Spawn_South"), ECWSSpawnDirection::South},
		{TEXT("CWS_Spawn_East"), ECWSSpawnDirection::East},
		{TEXT("CWS_Spawn_West"), ECWSSpawnDirection::West},
		{TEXT("CWS_Spawn_NorthEast"), ECWSSpawnDirection::NorthEast},
		{TEXT("CWS_Spawn_NorthWest"), ECWSSpawnDirection::NorthWest},
		{TEXT("CWS_Spawn_SouthEast"), ECWSSpawnDirection::SouthEast},
		{TEXT("CWS_Spawn_SouthWest"), ECWSSpawnDirection::SouthWest},
		{TEXT("CWS_Spawn_Center"), ECWSSpawnDirection::Center},
	};

	for (const TPair<FName, ECWSSpawnDirection>& DirectionTag : DirectionTags)
	{
		if (ActorHasTag(DirectionTag.Key))
		{
			return DirectionTag.Value;
		}
	}
	return Direction;
}
