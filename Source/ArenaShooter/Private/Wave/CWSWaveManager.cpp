#include "Wave/CWSWaveManager.h"

#include "Components/CWSHealthComponent.h"
#include "Enemy/CWSBossEnemy.h"
#include "Enemy/CWSEnemyBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Wave/CWSSpawnPoint.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSWave, Log, All);

namespace
{
	FCWSRoundSpawnGroup MakeSpawnGroup(
		const ECWSSpawnDirection Direction,
		const int32 Count,
		const float Interval,
		const bool bUseBossClass = false)
	{
		FCWSRoundSpawnGroup Group;
		Group.Direction = Direction;
		Group.Count = Count;
		Group.SpawnInterval = Interval;
		Group.bUseBossClass = bUseBossClass;
		return Group;
	}
}

ACWSWaveManager::ACWSWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultEnemyClass = ACWSEnemyBase::StaticClass();
	BossEnemyClass = ACWSBossEnemy::StaticClass();

	BuildDefaultRounds();
}

void ACWSWaveManager::BeginPlay()
{
	Super::BeginPlay();
	CacheSpawnPoints();
	if (bAutoStart)
	{
		GetWorldTimerManager().SetTimer(
			InitialStartTimerHandle,
			this,
			&ACWSWaveManager::StartWaveSystem,
			FMath::Max(InitialStartDelay, 0.01f),
			false);
	}
}

void ACWSWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWaveSystem();
	Super::EndPlay(EndPlayReason);
}

void ACWSWaveManager::StartWaveSystem()
{
	if (bWaveSystemStarted || Rounds.IsEmpty())
	{
		return;
	}

	CacheSpawnPoints();
	if (CachedSpawnPoints.IsEmpty())
	{
		UE_LOG(LogCWSWave, Error, TEXT("Wave system cannot start because no CWS spawn points were found."));
		return;
	}

	bWaveSystemStarted = true;
	bAllRoundsCompleted = false;
	StartRound(Rounds[0].RoundNumber);
}

void ACWSWaveManager::StopWaveSystem()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	for (const TWeakObjectPtr<AActor>& Enemy : AliveEnemies)
	{
		if (AActor* EnemyActor = Enemy.Get())
		{
			EnemyActor->OnDestroyed.RemoveDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDestroyed);
			if (UCWSHealthComponent* Health = EnemyActor->FindComponentByClass<UCWSHealthComponent>())
			{
				Health->OnDeath.RemoveDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDeath);
			}
		}
	}
	PendingSpawns.Reset();
	AliveEnemies.Reset();
	bRoundInProgress = false;
	bWaveSystemStarted = false;
}

void ACWSWaveManager::StartRound(const int32 RoundNumber)
{
	const FCWSRoundDefinition* RoundDefinition = FindRoundDefinition(RoundNumber);
	if (!RoundDefinition)
	{
		UE_LOG(LogCWSWave, Error, TEXT("Cannot start missing round %d."), RoundNumber);
		return;
	}

	GetWorldTimerManager().ClearTimer(PreRoundTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(PostRoundTimerHandle);
	AliveEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& Enemy) { return !Enemy.IsValid(); });
	if (!AliveEnemies.IsEmpty())
	{
		UE_LOG(LogCWSWave, Warning, TEXT("Round %d started while %d tracked enemies are still alive."), RoundNumber, AliveEnemies.Num());
	}

	CurrentRound = RoundNumber;
	bRoundInProgress = false;
	BuildSpawnQueue(*RoundDefinition);
	BroadcastRemainingEnemyCount();

	GetWorldTimerManager().SetTimer(
		PreRoundTimerHandle,
		this,
		&ACWSWaveManager::BeginCurrentRoundSpawning,
		FMath::Max(RoundDefinition->PreRoundDelay, 0.01f),
		false);

	UE_LOG(LogCWSWave, Log, TEXT("Round %d prepared with %d enemies."), CurrentRound, PendingSpawns.Num());
}

int32 ACWSWaveManager::GetRemainingEnemyCount() const
{
	int32 ValidAliveEnemies = 0;
	for (const TWeakObjectPtr<AActor>& Enemy : AliveEnemies)
	{
		if (Enemy.IsValid())
		{
			++ValidAliveEnemies;
		}
	}
	return PendingSpawns.Num() + ValidAliveEnemies;
}

void ACWSWaveManager::BuildDefaultRounds()
{
	Rounds.Reset();

	FCWSRoundDefinition Round1;
	Round1.RoundNumber = 1;
	Round1.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::North, 4, 1.2f),
		MakeSpawnGroup(ECWSSpawnDirection::South, 4, 1.2f),
	};
	Rounds.Add(Round1);

	FCWSRoundDefinition Round2;
	Round2.RoundNumber = 2;
	Round2.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::North, 4, 1.0f),
		MakeSpawnGroup(ECWSSpawnDirection::South, 4, 1.0f),
		MakeSpawnGroup(ECWSSpawnDirection::East, 4, 1.0f),
		MakeSpawnGroup(ECWSSpawnDirection::West, 4, 1.0f),
	};
	Rounds.Add(Round2);

	FCWSRoundDefinition Round3;
	Round3.RoundNumber = 3;
	Round3.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::North, 4, 0.9f),
		MakeSpawnGroup(ECWSSpawnDirection::South, 4, 0.9f),
		MakeSpawnGroup(ECWSSpawnDirection::East, 4, 0.9f),
		MakeSpawnGroup(ECWSSpawnDirection::West, 4, 0.9f),
		MakeSpawnGroup(ECWSSpawnDirection::NorthEast, 4, 1.1f),
		MakeSpawnGroup(ECWSSpawnDirection::SouthWest, 4, 1.1f),
	};
	Rounds.Add(Round3);

	FCWSRoundDefinition Round4;
	Round4.RoundNumber = 4;
	Round4.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::North, 5, 0.8f),
		MakeSpawnGroup(ECWSSpawnDirection::South, 5, 0.8f),
		MakeSpawnGroup(ECWSSpawnDirection::East, 4, 0.8f),
		MakeSpawnGroup(ECWSSpawnDirection::West, 4, 0.8f),
		MakeSpawnGroup(ECWSSpawnDirection::NorthEast, 4, 1.0f),
		MakeSpawnGroup(ECWSSpawnDirection::NorthWest, 4, 1.0f),
		MakeSpawnGroup(ECWSSpawnDirection::SouthEast, 4, 1.0f),
		MakeSpawnGroup(ECWSSpawnDirection::SouthWest, 4, 1.0f),
	};
	Rounds.Add(Round4);

	FCWSRoundDefinition Round5;
	Round5.RoundNumber = 5;
	Round5.PreRoundDelay = 5.0f;
	Round5.PostRoundDelay = 0.0f;
	Round5.bBossRound = true;
	Round5.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::Center, 1, 0.1f, true),
		MakeSpawnGroup(ECWSSpawnDirection::North, 3, 1.2f),
		MakeSpawnGroup(ECWSSpawnDirection::South, 3, 1.2f),
		MakeSpawnGroup(ECWSSpawnDirection::East, 2, 1.2f),
		MakeSpawnGroup(ECWSSpawnDirection::West, 2, 1.2f),
		MakeSpawnGroup(ECWSSpawnDirection::NorthEast, 1, 1.5f),
		MakeSpawnGroup(ECWSSpawnDirection::NorthWest, 1, 1.5f),
		MakeSpawnGroup(ECWSSpawnDirection::SouthEast, 1, 1.5f),
		MakeSpawnGroup(ECWSSpawnDirection::SouthWest, 1, 1.5f),
	};
	Rounds.Add(Round5);
}

void ACWSWaveManager::CacheSpawnPoints()
{
	CachedSpawnPoints.Reset();
	for (TActorIterator<ACWSSpawnPoint> It(GetWorld()); It; ++It)
	{
		if (It->CanSpawn())
		{
			CachedSpawnPoints.Add(*It);
		}
	}
	DirectionSpawnIndices.Reset();
	UE_LOG(LogCWSWave, Log, TEXT("Registered %d CWS spawn points."), CachedSpawnPoints.Num());
}

void ACWSWaveManager::BeginCurrentRoundSpawning()
{
	bRoundInProgress = true;
	OnRoundStarted.Broadcast(CurrentRound);
	UE_LOG(LogCWSWave, Log, TEXT("Round %d started."), CurrentRound);
	SpawnNextEnemy();
}

void ACWSWaveManager::BuildSpawnQueue(const FCWSRoundDefinition& RoundDefinition)
{
	PendingSpawns.Reset();
	int32 MaximumGroupCount = 0;
	for (const FCWSRoundSpawnGroup& Group : RoundDefinition.SpawnGroups)
	{
		MaximumGroupCount = FMath::Max(MaximumGroupCount, Group.Count);
	}

	for (int32 SpawnIndex = 0; SpawnIndex < MaximumGroupCount; ++SpawnIndex)
	{
		for (const FCWSRoundSpawnGroup& Group : RoundDefinition.SpawnGroups)
		{
			if (SpawnIndex < Group.Count)
			{
				FPendingSpawn& PendingSpawn = PendingSpawns.AddDefaulted_GetRef();
				PendingSpawn.Direction = Group.Direction;
				PendingSpawn.Interval = FMath::Max(Group.SpawnInterval, 0.05f);
				PendingSpawn.bUseBossClass = Group.bUseBossClass;
			}
		}
	}
}

void ACWSWaveManager::SpawnNextEnemy()
{
	if (!bRoundInProgress || PendingSpawns.IsEmpty())
	{
		EvaluateRoundCompletion();
		return;
	}

	const FPendingSpawn PendingSpawn = PendingSpawns[0];
	PendingSpawns.RemoveAt(0);
	ACWSSpawnPoint* SpawnPoint = SelectSpawnPoint(PendingSpawn.Direction);
	const TSoftClassPtr<APawn>& EnemyClassAsset =
		PendingSpawn.bUseBossClass && !BossEnemyClass.IsNull() ? BossEnemyClass : DefaultEnemyClass;
	UClass* EnemyClass = EnemyClassAsset.LoadSynchronous();

	if (!SpawnPoint)
	{
		UE_LOG(LogCWSWave, Error, TEXT("No enabled spawn point for direction %s."), *UEnum::GetValueAsString(PendingSpawn.Direction));
		StopWaveSystem();
		return;
	}
	else if (!EnemyClass)
	{
		UE_LOG(LogCWSWave, Error, TEXT("No enemy class configured for round %d."), CurrentRound);
		StopWaveSystem();
		return;
	}
	else
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APawn* SpawnedEnemy = GetWorld()->SpawnActor<APawn>(EnemyClass, SpawnPoint->GetSpawnTransform(), SpawnParameters);
		if (SpawnedEnemy)
		{
			if (!SpawnedEnemy->GetController())
			{
				SpawnedEnemy->SpawnDefaultController();
			}
			SpawnedEnemy->OnDestroyed.AddDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDestroyed);
			if (UCWSHealthComponent* Health = SpawnedEnemy->FindComponentByClass<UCWSHealthComponent>())
			{
				Health->OnDeath.AddDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDeath);
			}
			AliveEnemies.Add(SpawnedEnemy);
			if (PendingSpawn.bUseBossClass)
			{
				if (ACWSBossEnemy* Boss = Cast<ACWSBossEnemy>(SpawnedEnemy))
				{
					OnBossSpawned.Broadcast(Boss);
					UE_LOG(LogCWSWave, Display, TEXT("Round %d spawned the dedicated boss."), CurrentRound);
				}
				else
				{
					UE_LOG(LogCWSWave, Error, TEXT("Boss slot spawned a non-boss class: %s."), *GetNameSafe(EnemyClass));
				}
			}
			UE_LOG(
				LogCWSWave,
				Log,
				TEXT("Round %d spawned %s from %s. Remaining=%d"),
				CurrentRound,
				*GetNameSafe(EnemyClass),
				*UEnum::GetValueAsString(PendingSpawn.Direction),
				GetRemainingEnemyCount());
		}
		else
		{
			UE_LOG(LogCWSWave, Error, TEXT("Failed to spawn enemy for round %d."), CurrentRound);
			StopWaveSystem();
			return;
		}
	}

	BroadcastRemainingEnemyCount();
	if (!PendingSpawns.IsEmpty())
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&ACWSWaveManager::SpawnNextEnemy,
			PendingSpawn.Interval,
			false);
	}
	else
	{
		EvaluateRoundCompletion();
	}
}

void ACWSWaveManager::EvaluateRoundCompletion()
{
	AliveEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& Enemy) { return !Enemy.IsValid(); });
	if (bRoundInProgress && PendingSpawns.IsEmpty() && AliveEnemies.IsEmpty())
	{
		CompleteCurrentRound();
	}
}

void ACWSWaveManager::CompleteCurrentRound()
{
	bRoundInProgress = false;
	OnRoundCleared.Broadcast(CurrentRound);
	BroadcastRemainingEnemyCount();
	UE_LOG(LogCWSWave, Log, TEXT("Round %d cleared."), CurrentRound);

	const int32 CurrentRoundIndex = Rounds.IndexOfByPredicate(
		[this](const FCWSRoundDefinition& Definition) { return Definition.RoundNumber == CurrentRound; });
	if (CurrentRoundIndex == INDEX_NONE || CurrentRoundIndex + 1 >= Rounds.Num())
	{
		bAllRoundsCompleted = true;
		bWaveSystemStarted = false;
		OnAllRoundsCompleted.Broadcast();
		UE_LOG(LogCWSWave, Log, TEXT("All rounds completed."));
		return;
	}

	const float Delay = FMath::Max(Rounds[CurrentRoundIndex].PostRoundDelay, 0.01f);
	GetWorldTimerManager().SetTimer(PostRoundTimerHandle, this, &ACWSWaveManager::StartNextRound, Delay, false);
}

void ACWSWaveManager::StartNextRound()
{
	const int32 CurrentRoundIndex = Rounds.IndexOfByPredicate(
		[this](const FCWSRoundDefinition& Definition) { return Definition.RoundNumber == CurrentRound; });
	if (Rounds.IsValidIndex(CurrentRoundIndex + 1))
	{
		StartRound(Rounds[CurrentRoundIndex + 1].RoundNumber);
	}
}

void ACWSWaveManager::BroadcastRemainingEnemyCount()
{
	OnRemainingEnemyCountChanged.Broadcast(GetRemainingEnemyCount(), CurrentRound);
}

const FCWSRoundDefinition* ACWSWaveManager::FindRoundDefinition(const int32 RoundNumber) const
{
	return Rounds.FindByPredicate(
		[RoundNumber](const FCWSRoundDefinition& Definition) { return Definition.RoundNumber == RoundNumber; });
}

ACWSSpawnPoint* ACWSWaveManager::SelectSpawnPoint(const ECWSSpawnDirection Direction)
{
	TArray<ACWSSpawnPoint*> MatchingPoints;
	for (ACWSSpawnPoint* SpawnPoint : CachedSpawnPoints)
	{
		if (IsValid(SpawnPoint) && SpawnPoint->CanSpawn() && SpawnPoint->GetSpawnDirection() == Direction)
		{
			MatchingPoints.Add(SpawnPoint);
		}
	}

	if (MatchingPoints.IsEmpty())
	{
		return nullptr;
	}

	int32& SpawnIndex = DirectionSpawnIndices.FindOrAdd(Direction);
	ACWSSpawnPoint* SelectedPoint = MatchingPoints[SpawnIndex % MatchingPoints.Num()];
	++SpawnIndex;
	return SelectedPoint;
}

void ACWSWaveManager::HandleSpawnedEnemyDestroyed(AActor* DestroyedActor)
{
	RemoveTrackedEnemy(DestroyedActor);
}

void ACWSWaveManager::HandleSpawnedEnemyDeath(AActor* DeadActor)
{
	if (DeadActor)
	{
		DeadActor->OnDestroyed.RemoveDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDestroyed);
	}
	RemoveTrackedEnemy(DeadActor);
}

void ACWSWaveManager::RemoveTrackedEnemy(AActor* EnemyActor)
{
	const int32 RemovedCount = AliveEnemies.RemoveAll(
		[EnemyActor](const TWeakObjectPtr<AActor>& Enemy)
		{
			return !Enemy.IsValid() || Enemy.Get() == EnemyActor;
		});
	if (RemovedCount <= 0)
	{
		return;
	}

	BroadcastRemainingEnemyCount();
	UE_LOG(LogCWSWave, Log, TEXT("Enemy removed from round %d. Remaining=%d"), CurrentRound, GetRemainingEnemyCount());
	EvaluateRoundCompletion();
}
