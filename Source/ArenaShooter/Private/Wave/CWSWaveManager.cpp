#include "Wave/CWSWaveManager.h"

#include "Components/CWSHealthComponent.h"
#include "Enemy/CWSBossEnemy.h"
#include "Enemy/CWSEnemyBase.h"
#include "Enemy/CWSFastEnemy.h"
#include "Enemy/CWSTankEnemy.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Wave/CWSSpawnPoint.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSWave, Log, All);

namespace
{
	// 기본 라운드 테이블을 읽기 쉬운 선언 형태로 만드는 로컬 팩토리입니다.
	FCWSRoundSpawnGroup MakeSpawnGroup(
		const ECWSSpawnDirection Direction,
		const int32 Count,
		const float Interval,
		const ECWSEnemyType EnemyType = ECWSEnemyType::Normal)
	{
		FCWSRoundSpawnGroup Group;
		Group.Direction = Direction;
		Group.Count = Count;
		Group.SpawnInterval = Interval;
		Group.EnemyType = EnemyType;
		Group.bUseBossClass = EnemyType == ECWSEnemyType::Boss;
		return Group;
	}

	// 기존 맵에 저장된 SpawnGroup을 다시 저장하지 않아도 현재 5라운드의
	// Normal/Fast/Tank/Boss 조합을 적용하기 위한 호환 매핑입니다.
	ECWSEnemyType GetDefaultEnemyType(
		const int32 RoundNumber,
		const ECWSSpawnDirection Direction,
		const bool bUseBossClass)
	{
		if (bUseBossClass || Direction == ECWSSpawnDirection::Center)
		{
			return ECWSEnemyType::Boss;
		}
		if ((RoundNumber == 2 && (Direction == ECWSSpawnDirection::East || Direction == ECWSSpawnDirection::West)) ||
			(RoundNumber == 3 && (Direction == ECWSSpawnDirection::NorthEast || Direction == ECWSSpawnDirection::SouthWest)) ||
			(RoundNumber >= 4 && (Direction == ECWSSpawnDirection::North || Direction == ECWSSpawnDirection::South)))
		{
			return ECWSEnemyType::Fast;
		}
		if ((RoundNumber == 4 &&
			(Direction == ECWSSpawnDirection::East || Direction == ECWSSpawnDirection::West)) ||
			(RoundNumber == 3 && (Direction == ECWSSpawnDirection::North || Direction == ECWSSpawnDirection::South)) ||
			(RoundNumber == 5 && (Direction == ECWSSpawnDirection::East || Direction == ECWSSpawnDirection::West)))
		{
			return ECWSEnemyType::Tank;
		}
		return ECWSEnemyType::Normal;
	}
}

ACWSWaveManager::ACWSWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultEnemyClass = ACWSEnemyBase::StaticClass();
	FastEnemyClass = ACWSFastEnemy::StaticClass();
	TankEnemyClass = ACWSTankEnemy::StaticClass();
	BossEnemyClass = ACWSBossEnemy::StaticClass();

	BuildDefaultRounds();
}

void ACWSWaveManager::BeginPlay()
{
	Super::BeginPlay();
	CacheSpawnPoints();
	const ACWSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACWSGameMode>() : nullptr;
	// Title 화면이 있는 GameMode에서는 StartGame이 명시적으로 시작한다.
	// 다른 테스트 월드에서 GameMode가 없을 때는 bAutoStart 설정을 그대로 존중한다.
	if (bAutoStart && (!GameMode || GameMode->IsGameStarted()))
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
	// Stop은 GameOver와 EndPlay에서 모두 호출되므로 타이머와 delegate를 멱등하게 정리한다.
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
	if (CurrentPhase != ECWSWavePhase::Completed)
	{
		SetWavePhase(ECWSWavePhase::Stopped);
	}
}

void ACWSWaveManager::StartRound(const int32 RoundNumber)
{
	const FCWSRoundDefinition* RoundDefinition = FindRoundDefinition(RoundNumber);
	if (!RoundDefinition)
	{
		UE_LOG(LogCWSWave, Error, TEXT("Cannot start missing round %d."), RoundNumber);
		return;
	}

	// 이전 단계에서 남은 콜백이 새 라운드에 침투하지 않도록 모든 phase 타이머를 먼저 지운다.
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

	// Preparing 동안 HUD가 countdown을 그릴 수 있고, 타이머 종료 뒤에만 Active로 전환한다.
	GetWorldTimerManager().SetTimer(
		PreRoundTimerHandle,
		this,
		&ACWSWaveManager::BeginCurrentRoundSpawning,
		FMath::Max(RoundDefinition->PreRoundDelay, 0.01f),
		false);
	SetWavePhase(ECWSWavePhase::Preparing);

	UE_LOG(LogCWSWave, Log, TEXT("Round %d prepared with %d enemies."), CurrentRound, PendingSpawns.Num());
}

int32 ACWSWaveManager::GetRemainingEnemyCount() const
{
	// 아직 생성되지 않은 큐와 현재 살아 있는 Actor를 모두 포함해야 HUD 수가 줄지 않는다.
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

float ACWSWaveManager::GetPhaseTimeRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	float Remaining = 0.0f;
	if (CurrentPhase == ECWSWavePhase::Preparing)
	{
		Remaining = World->GetTimerManager().GetTimerRemaining(PreRoundTimerHandle);
	}
	else if (CurrentPhase == ECWSWavePhase::RoundCleared)
	{
		Remaining = World->GetTimerManager().GetTimerRemaining(PostRoundTimerHandle);
	}
	return FMath::Max(Remaining, 0.0f);
}

float ACWSWaveManager::GetPhaseElapsedTime() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(World->GetTimeSeconds() - PhaseStartedTime, 0.0f) : 0.0f;
}

void ACWSWaveManager::BuildDefaultRounds()
{
	// 생성자 기본값입니다. 에디터/Blueprint에서 저장한 Rounds 값은 직렬화 과정에서 덮어씁니다.
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
		MakeSpawnGroup(ECWSSpawnDirection::East, 4, 1.0f, ECWSEnemyType::Fast),
		MakeSpawnGroup(ECWSSpawnDirection::West, 4, 1.0f, ECWSEnemyType::Fast),
	};
	Rounds.Add(Round2);

	FCWSRoundDefinition Round3;
	Round3.RoundNumber = 3;
	Round3.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::North, 4, 0.9f, ECWSEnemyType::Tank),
		MakeSpawnGroup(ECWSSpawnDirection::South, 4, 0.9f, ECWSEnemyType::Tank),
		MakeSpawnGroup(ECWSSpawnDirection::East, 4, 0.9f),
		MakeSpawnGroup(ECWSSpawnDirection::West, 4, 0.9f),
		MakeSpawnGroup(ECWSSpawnDirection::NorthEast, 4, 1.1f, ECWSEnemyType::Fast),
		MakeSpawnGroup(ECWSSpawnDirection::SouthWest, 4, 1.1f, ECWSEnemyType::Fast),
	};
	Rounds.Add(Round3);

	FCWSRoundDefinition Round4;
	Round4.RoundNumber = 4;
	Round4.SpawnGroups = {
		MakeSpawnGroup(ECWSSpawnDirection::North, 5, 0.8f, ECWSEnemyType::Fast),
		MakeSpawnGroup(ECWSSpawnDirection::South, 5, 0.8f, ECWSEnemyType::Fast),
		MakeSpawnGroup(ECWSSpawnDirection::East, 4, 0.8f, ECWSEnemyType::Tank),
		MakeSpawnGroup(ECWSSpawnDirection::West, 4, 0.8f, ECWSEnemyType::Tank),
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
		MakeSpawnGroup(ECWSSpawnDirection::Center, 1, 0.1f, ECWSEnemyType::Boss),
		MakeSpawnGroup(ECWSSpawnDirection::North, 3, 1.2f, ECWSEnemyType::Fast),
		MakeSpawnGroup(ECWSSpawnDirection::South, 3, 1.2f, ECWSEnemyType::Fast),
		MakeSpawnGroup(ECWSSpawnDirection::East, 2, 1.2f, ECWSEnemyType::Tank),
		MakeSpawnGroup(ECWSSpawnDirection::West, 2, 1.2f, ECWSEnemyType::Tank),
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
	SetWavePhase(ECWSWavePhase::Active);
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

	// 그룹 하나를 끝까지 생성한 뒤 다음 방향으로 넘어가지 않고, 같은 인덱스의 항목을
	// 방향별로 한 번씩 넣어 Arena 한쪽에 적이 몰리는 것을 줄인다.
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
				PendingSpawn.EnemyType = bUseDefaultArchetypeComposition
					? GetDefaultEnemyType(CurrentRound, Group.Direction, Group.bUseBossClass)
					: Group.bUseBossClass ? ECWSEnemyType::Boss : Group.EnemyType;
			}
		}
	}
}

UClass* ACWSWaveManager::ResolveEnemyClass(const ECWSEnemyType EnemyType) const
{
	const TSoftClassPtr<APawn>* EnemyClassAsset = &DefaultEnemyClass;
	switch (EnemyType)
	{
	case ECWSEnemyType::Fast:
		EnemyClassAsset = &FastEnemyClass;
		break;
	case ECWSEnemyType::Tank:
		EnemyClassAsset = &TankEnemyClass;
		break;
	case ECWSEnemyType::Boss:
		EnemyClassAsset = &BossEnemyClass;
		break;
	default:
		break;
	}
	return EnemyClassAsset->LoadSynchronous();
}

void ACWSWaveManager::SpawnNextEnemy()
{
	if (!bRoundInProgress || PendingSpawns.IsEmpty())
	{
		EvaluateRoundCompletion();
		return;
	}

	// 항목을 먼저 큐에서 제거하므로 GetRemainingEnemyCount는 스폰 성공 뒤 AliveEnemies와
	// 합쳐도 같은 적을 두 번 세지 않는다.
	const FPendingSpawn PendingSpawn = PendingSpawns[0];
	PendingSpawns.RemoveAt(0);
	ACWSSpawnPoint* SpawnPoint = SelectSpawnPoint(PendingSpawn.Direction);
	UClass* EnemyClass = ResolveEnemyClass(PendingSpawn.EnemyType);

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
			// OnDeath가 정상 클리어의 주 경로이고, 외부 Destroy는 안전망으로 추적한다.
			SpawnedEnemy->OnDestroyed.AddDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDestroyed);
			if (UCWSHealthComponent* Health = SpawnedEnemy->FindComponentByClass<UCWSHealthComponent>())
			{
				Health->OnDeath.AddDynamic(this, &ACWSWaveManager::HandleSpawnedEnemyDeath);
			}
			AliveEnemies.Add(SpawnedEnemy);
			if (PendingSpawn.EnemyType == ECWSEnemyType::Boss)
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
				TEXT("Round %d spawned %s (%s) from %s. Remaining=%d"),
				CurrentRound,
				*GetNameSafe(EnemyClass),
				*UEnum::GetValueAsString(PendingSpawn.EnemyType),
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
	// 마지막 스폰과 마지막 적 사망 조건이 모두 만족해야 라운드를 끝낸다.
	AliveEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& Enemy) { return !Enemy.IsValid(); });
	if (bRoundInProgress && PendingSpawns.IsEmpty() && AliveEnemies.IsEmpty())
	{
		CompleteCurrentRound();
	}
}

void ACWSWaveManager::CompleteCurrentRound()
{
	bRoundInProgress = false;
	const int32 CurrentRoundIndex = Rounds.IndexOfByPredicate(
		[this](const FCWSRoundDefinition& Definition) { return Definition.RoundNumber == CurrentRound; });
	if (CurrentRoundIndex == INDEX_NONE || CurrentRoundIndex + 1 >= Rounds.Num())
	{
		// 마지막 라운드는 중간 RoundCleared 대기 없이 Completed를 최종 상태로 유지한다.
		bAllRoundsCompleted = true;
		bWaveSystemStarted = false;
		SetWavePhase(ECWSWavePhase::Completed);
		OnRoundCleared.Broadcast(CurrentRound);
		BroadcastRemainingEnemyCount();
		UE_LOG(LogCWSWave, Log, TEXT("Round %d cleared."), CurrentRound);
		OnAllRoundsCompleted.Broadcast();
		UE_LOG(LogCWSWave, Log, TEXT("All rounds completed."));
		return;
	}

	// 중간 라운드는 보급 수집과 HUD 안내를 위한 RoundCleared 구간을 둔다.
	const float Delay = FMath::Max(Rounds[CurrentRoundIndex].PostRoundDelay, 0.01f);
	GetWorldTimerManager().SetTimer(PostRoundTimerHandle, this, &ACWSWaveManager::StartNextRound, Delay, false);
	SetWavePhase(ECWSWavePhase::RoundCleared);
	OnRoundCleared.Broadcast(CurrentRound);
	BroadcastRemainingEnemyCount();
	UE_LOG(LogCWSWave, Log, TEXT("Round %d cleared."), CurrentRound);
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

void ACWSWaveManager::SetWavePhase(const ECWSWavePhase NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}

	// 진입 시각을 함께 기록해 HUD가 별도 타이머 없이 Active 안내 노출 시간을 계산한다.
	CurrentPhase = NewPhase;
	PhaseStartedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	OnWavePhaseChanged.Broadcast(CurrentPhase, CurrentRound);
	UE_LOG(
		LogCWSWave,
		Display,
		TEXT("CWS_WAVE_PHASE: %s Round=%d"),
		*UEnum::GetValueAsString(CurrentPhase),
		CurrentRound);
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

	// 동일 방향 SpawnPoint가 여러 개인 맵에서도 매번 첫 지점만 사용하지 않는다.
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
		// 사망 뒤 이어지는 Destroy가 같은 적을 다시 제거하지 않도록 안전망 delegate를 해제한다.
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
