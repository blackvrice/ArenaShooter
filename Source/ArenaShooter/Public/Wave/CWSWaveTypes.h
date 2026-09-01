#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"
#include "CWSWaveTypes.generated.h"

UENUM(BlueprintType)
enum class ECWSSpawnDirection : uint8
{
	North UMETA(DisplayName = "North"),
	South UMETA(DisplayName = "South"),
	East UMETA(DisplayName = "East"),
	West UMETA(DisplayName = "West"),
	NorthEast UMETA(DisplayName = "North East"),
	NorthWest UMETA(DisplayName = "North West"),
	SouthEast UMETA(DisplayName = "South East"),
	SouthWest UMETA(DisplayName = "South West"),
	Center UMETA(DisplayName = "Center")
};

/** HUD와 GameMode가 관찰하는 WaveManager의 명시적 상태 머신입니다. */
UENUM(BlueprintType)
enum class ECWSWavePhase : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Preparing UMETA(DisplayName = "Preparing"),
	Active UMETA(DisplayName = "Active"),
	RoundCleared UMETA(DisplayName = "Round Cleared"),
	Completed UMETA(DisplayName = "Completed"),
	Stopped UMETA(DisplayName = "Stopped")
};

/** 같은 방향/적 타입/간격을 공유하는 라운드 내 스폰 묶음입니다. */
USTRUCT(BlueprintType)
struct ARENASHOOTER_API FCWSRoundSpawnGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	ECWSSpawnDirection Direction = ECWSSpawnDirection::North;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.05"))
	float SpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	ECWSEnemyType EnemyType = ECWSEnemyType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bUseBossClass = false;
};

/** 준비 시간부터 클리어 후 대기 시간까지 한 라운드의 전체 규칙입니다. */
USTRUCT(BlueprintType)
struct ARENASHOOTER_API FCWSRoundDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1"))
	int32 RoundNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FCWSRoundSpawnGroup> SpawnGroups;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.0"))
	float PreRoundDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.0"))
	float PostRoundDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bBossRound = false;
};
