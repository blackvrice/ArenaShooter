#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Wave/CWSWaveTypes.h"
#include "CWSSpawnPoint.generated.h"

class USceneComponent;

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
