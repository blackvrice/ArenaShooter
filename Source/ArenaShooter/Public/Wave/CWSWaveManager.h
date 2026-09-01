#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Wave/CWSWaveTypes.h"
#include "CWSWaveManager.generated.h"

class ACWSSpawnPoint;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSRoundEvent, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSRemainingEnemyEvent, int32, RemainingEnemies, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSWavePhaseEvent, ECWSWavePhase, WavePhase, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCWSWaveSystemCompletedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSBossSpawnedEvent, AActor*, BossActor);

/**
 * 라운드 정의를 시간 순서대로 실행하고 살아 있는 적을 추적하는 Wave 상태 머신입니다.
 *
 * 핵심 흐름은 Preparing -> Active -> RoundCleared -> 다음 Preparing이며,
 * 마지막 라운드는 Completed로 끝납니다. 적의 HealthComponent OnDeath를 주 경로로,
 * OnDestroyed를 안전망으로 사용해 남은 적 수가 중복 차감되지 않도록 관리합니다.
 */
UCLASS(BlueprintType)
class ARENASHOOTER_API ACWSWaveManager : public AActor
{
	GENERATED_BODY()

public:
	ACWSWaveManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 첫 라운드부터 전체 웨이브 흐름을 시작합니다. 이미 시작했으면 아무 일도 하지 않습니다. */
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void StartWaveSystem();

	/** 타이머와 적 이벤트 바인딩을 정리하고 추가 스폰을 중단합니다. */
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void StopWaveSystem();

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void StartRound(int32 RoundNumber);

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetRemainingEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	bool IsRoundInProgress() const { return bRoundInProgress; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	bool IsWaveSystemStarted() const { return bWaveSystemStarted; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	ECWSWavePhase GetWavePhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetPhaseTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetPhaseElapsedTime() const;

	// UI와 GameMode는 Tick으로 상태를 추측하지 않고 아래 이벤트를 관찰합니다.
	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSRoundEvent OnRoundStarted;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSRoundEvent OnRoundCleared;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSRemainingEnemyEvent OnRemainingEnemyCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSWavePhaseEvent OnWavePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSWaveSystemCompletedEvent OnAllRoundsCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSBossSpawnedEvent OnBossSpawned;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.0"))
	float InitialStartDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TSoftClassPtr<APawn> DefaultEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TSoftClassPtr<APawn> FastEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TSoftClassPtr<APawn> TankEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TSoftClassPtr<APawn> BossEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bUseDefaultArchetypeComposition = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FCWSRoundDefinition> Rounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	int32 CurrentRound = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bRoundInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bAllRoundsCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	ECWSWavePhase CurrentPhase = ECWSWavePhase::Idle;

private:
	/** 에디터용 정의를 실행 시점의 한 건 단위 작업으로 펼친 내부 큐 항목입니다. */
	struct FPendingSpawn
	{
		ECWSSpawnDirection Direction = ECWSSpawnDirection::North;
		ECWSEnemyType EnemyType = ECWSEnemyType::Normal;
		float Interval = 1.0f;
		bool bUseBossClass = false;
	};

	void BuildDefaultRounds();
	void CacheSpawnPoints();
	void BeginCurrentRoundSpawning();
	void BuildSpawnQueue(const FCWSRoundDefinition& RoundDefinition);
	UClass* ResolveEnemyClass(ECWSEnemyType EnemyType) const;
	void SpawnNextEnemy();
	void EvaluateRoundCompletion();
	void CompleteCurrentRound();
	void StartNextRound();
	void SetWavePhase(ECWSWavePhase NewPhase);
	void BroadcastRemainingEnemyCount();
	const FCWSRoundDefinition* FindRoundDefinition(int32 RoundNumber) const;
	ACWSSpawnPoint* SelectSpawnPoint(ECWSSpawnDirection Direction);

	UFUNCTION()
	void HandleSpawnedEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleSpawnedEnemyDeath(AActor* DeadActor);

	void RemoveTrackedEnemy(AActor* EnemyActor);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACWSSpawnPoint>> CachedSpawnPoints;

	// Weak 참조를 사용해 외부에서 Destroy된 적이 수명을 연장하지 않도록 합니다.
	TArray<TWeakObjectPtr<AActor>> AliveEnemies;
	TArray<FPendingSpawn> PendingSpawns;
	// 같은 방향에 SpawnPoint가 여러 개면 매번 다음 지점을 고르는 라운드 로빈 인덱스입니다.
	TMap<ECWSSpawnDirection, int32> DirectionSpawnIndices;
	FTimerHandle InitialStartTimerHandle;
	FTimerHandle PreRoundTimerHandle;
	FTimerHandle SpawnTimerHandle;
	FTimerHandle PostRoundTimerHandle;
	bool bWaveSystemStarted = false;
	float PhaseStartedTime = 0.0f;
};
