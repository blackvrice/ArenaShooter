#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Wave/CWSWaveTypes.h"
#include "CWSSpawnPoint.generated.h"

class USceneComponent;

/** WaveManager가 방향별 스폰 위치를 찾을 때 사용하는 가벼운 레벨 마커입니다. */
UCLASS(BlueprintType)
class ARENASHOOTER_API ACWSSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ACWSSpawnPoint();

	UFUNCTION(BlueprintPure, Category = "Wave|Spawn")
	FTransform GetSpawnTransform() const;

	UFUNCTION(BlueprintPure, Category = "Wave|Spawn")
	bool CanSpawn() const;

	UFUNCTION(BlueprintPure, Category = "Wave|Spawn")
	ECWSSpawnDirection GetSpawnDirection() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave|Spawn")
	ECWSSpawnDirection Direction = ECWSSpawnDirection::North;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave|Spawn")
	bool bEnabled = true;

private:
	UPROPERTY(VisibleAnywhere, Category = "Wave|Spawn")
	TObjectPtr<USceneComponent> SceneRoot;
};
