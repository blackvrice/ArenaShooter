#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Wave/CWSWaveTypes.h"
#include "CWSWaveManager.generated.h"

class ACWSSpawnPoint;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSRoundEvent, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSRemainingEnemyEvent, int32, RemainingEnemies, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCWSWaveSystemCompletedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSBossSpawnedEvent, AActor*, BossActor);

UCLASS(BlueprintType)
class ARENASHOOTER_API ACWSWaveManager : public AActor
{
	GENERATED_BODY()

public:
	ACWSWaveManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void StartWaveSystem();

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

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSRoundEvent OnRoundStarted;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSRoundEvent OnRoundCleared;

	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FCWSRemainingEnemyEvent OnRemainingEnemyCountChanged;

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
	TSoftClassPtr<APawn> BossEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FCWSRoundDefinition> Rounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	int32 CurrentRound = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bRoundInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	bool bAllRoundsCompleted = false;

private:
	struct FPendingSpawn
	{
		ECWSSpawnDirection Direction = ECWSSpawnDirection::North;
		float Interval = 1.0f;
		bool bUseBossClass = false;
	};

	void BuildDefaultRounds();
	void CacheSpawnPoints();
	void BeginCurrentRoundSpawning();
	void BuildSpawnQueue(const FCWSRoundDefinition& RoundDefinition);
	void SpawnNextEnemy();
	void EvaluateRoundCompletion();
	void CompleteCurrentRound();
	void StartNextRound();
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

	TArray<TWeakObjectPtr<AActor>> AliveEnemies;
	TArray<FPendingSpawn> PendingSpawns;
	TMap<ECWSSpawnDirection, int32> DirectionSpawnIndices;
	FTimerHandle InitialStartTimerHandle;
	FTimerHandle PreRoundTimerHandle;
	FTimerHandle SpawnTimerHandle;
	FTimerHandle PostRoundTimerHandle;
	bool bWaveSystemStarted = false;
};
