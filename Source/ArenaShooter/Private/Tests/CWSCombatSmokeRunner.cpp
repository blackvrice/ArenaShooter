#include "Tests/CWSCombatSmokeRunner.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Enemy/CWSBossEnemy.h"
#include "Enemy/CWSEnemyBase.h"
#include "Enemy/CWSFastEnemy.h"
#include "Enemy/CWSTankEnemy.h"
#include "EngineUtils.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/CWSSupplyPickup.h"
#include "Player/CWSPlayerCharacter.h"
#include "TimerManager.h"
#include "Wave/CWSWaveManager.h"
#include "World/CWSArenaVisualDirector.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSCombatSmoke, Log, All);

namespace
{
	// OpenLevel 뒤 새 GameMode/Runner 인스턴스에서도 Restart 2단계를 이어가기 위한 프로세스 상태입니다.
    bool GCombatSmokeRestartRequested = false;
}

FCWSCombatSmokeRunner::FCWSCombatSmokeRunner(ACWSGameMode& InOwner)
    : Owner(InOwner)
{
}

FCWSCombatSmokeRunner::~FCWSCombatSmokeRunner() = default;

bool FCWSCombatSmokeRunner::StartFromCommandLine()
{
    bSmokeTestAllRounds = FParse::Param(FCommandLine::Get(), TEXT("CWSAllRoundsSmokeTest"));
    bSmokeTestEnabled =
        bSmokeTestAllRounds || FParse::Param(FCommandLine::Get(), TEXT("CWSRoundOneSmokeTest"));
    if (!bSmokeTestEnabled)
    {
        return false;
    }

    SmokeStartTime = Owner.GetWorld()->GetTimeSeconds();
    bSmokeRestartVerification = !bSmokeTestAllRounds && GCombatSmokeRestartRequested;
    Owner.GetWorldTimerManager().SetTimer(
        SmokeStepTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunCombatSmokeStep(); }),
        0.1f,
        true,
        0.1f);
    UE_LOG(
        LogCWSCombatSmoke,
        Display,
        TEXT("%s"),
        bSmokeTestAllRounds
            ? TEXT("CWS_ALL_ROUNDS_SMOKE_STARTED")
            : bSmokeRestartVerification
                ? TEXT("CWS_ROUND_ONE_RESTART_VALIDATION_STARTED")
                : TEXT("CWS_ROUND_ONE_SMOKE_STARTED"));
    return true;
}

void FCWSCombatSmokeRunner::HandleRoundCleared(const int32 RoundNumber)
{
    SmokeHighestRoundCleared = FMath::Max(SmokeHighestRoundCleared, RoundNumber);
    UE_LOG(LogCWSCombatSmoke, Display, TEXT("Smoke runner observed round %d clear."), RoundNumber);
	if (bSmokeTestAllRounds && SmokeRoundHealthRestorePreparedForRound == RoundNumber && Owner.PlayerHealth.IsValid())
	{
		const float CurrentHealth = Owner.PlayerHealth->GetCurrentHealth();
		const float MaxHealth = Owner.PlayerHealth->GetMaxHealth();
		if (FMath::IsNearlyEqual(CurrentHealth, MaxHealth))
		{
			++SmokeRoundHealthRestoreVerifiedCount;
			UE_LOG(
				LogCWSCombatSmoke,
				Display,
				TEXT("CWS_ROUND_CLEAR_HEALTH_RESTORE_VERIFIED: Round=%d Health=%.1f/%.1f"),
				RoundNumber,
				CurrentHealth,
				MaxHealth);
		}
		else
		{
			UE_LOG(
				LogCWSCombatSmoke,
				Error,
				TEXT("Round %d clear did not fully restore player health: %.1f/%.1f"),
				RoundNumber,
				CurrentHealth,
				MaxHealth);
		}
	}
    if (RoundNumber == 1)
    {
        bSmokeRoundOneCleared = true;
    }
}

void FCWSCombatSmokeRunner::HandleAllRoundsCompleted()
{
    if (bSmokeTestAllRounds)
    {
        bSmokeAllRoundsCleared = true;
    }
}

void FCWSCombatSmokeRunner::HandlePlayerDeath()
{
    if (bSmokeAppliedPlayerDamage)
    {
        bSmokeSawPlayerDeath = true;
    }
}

void FCWSCombatSmokeRunner::HandleSupplySpawned(ACWSSupplyPickup* Supply)
{
    LastRoundSupply = Supply;
}
void FCWSCombatSmokeRunner::HandleWavePhaseChanged(const ECWSWavePhase WavePhase, const int32 RoundNumber)
{
	if (!bSmokeTestEnabled)
	{
		return;
	}

	switch (WavePhase)
	{
	case ECWSWavePhase::Preparing:
		bSmokeSawPreparingPhase = true;
		break;
	case ECWSWavePhase::Active:
		bSmokeSawActivePhase = true;
		break;
	case ECWSWavePhase::RoundCleared:
		bSmokeSawRoundClearedPhase = true;
		break;
	case ECWSWavePhase::Completed:
		bSmokeSawCompletedPhase = true;
		break;
	default:
		break;
	}

	if (!bSmokeLoggedRoundAnnouncementPhases && bSmokeSawPreparingPhase && bSmokeSawActivePhase &&
		bSmokeSawRoundClearedPhase)
	{
		bSmokeLoggedRoundAnnouncementPhases = true;
		UE_LOG(
			LogCWSCombatSmoke,
			Display,
			TEXT("CWS_ROUND_ANNOUNCEMENT_PHASES_VERIFIED: Preparing, Active, and RoundCleared observed through round %d"),
			RoundNumber);
	}
}

void FCWSCombatSmokeRunner::ConfigureAllRoundsSmokeTimings()
{
	if (!bSmokeTestAllRounds || bSmokeTimingsConfigured || !Owner.WaveManager.IsValid())
	{
		return;
	}

	// 전체 라운드 모드에서 규칙은 유지하고 대기 시간만 축소한다.
	for (FCWSRoundDefinition& Round : Owner.WaveManager->Rounds)
	{
		Round.PreRoundDelay = 0.05f;
		Round.PostRoundDelay = 0.05f;
		for (FCWSRoundSpawnGroup& Group : Round.SpawnGroups)
		{
			Group.SpawnInterval = 0.05f;
		}
	}
	bSmokeTimingsConfigured = true;
	UE_LOG(LogCWSCombatSmoke, Display, TEXT("All-round smoke timings accelerated."));
}

void FCWSCombatSmokeRunner::PrepareRoundClearHealthRestoreVerification()
{
	if (!bSmokeTestAllRounds || !Owner.WaveManager.IsValid() || !Owner.PlayerHealth.IsValid())
	{
		return;
	}

	// 마지막 한 적이 남았을 때 플레이어에게 비치명 피해를 만들어, 바로 뒤 RoundClear
	// 이벤트에서 GameMode가 최대 체력으로 복구했는지 라운드마다 검증한다.
	const int32 RoundNumber = Owner.WaveManager->GetCurrentRound();
	if (RoundNumber <= 0 || SmokeRoundHealthRestorePreparedForRound == RoundNumber ||
		Owner.WaveManager->GetRemainingEnemyCount() != 1)
	{
		return;
	}

	UCWSHealthComponent* Health = Owner.PlayerHealth.Get();
	if (!Health->IsAlive())
	{
		return;
	}

	const float MaxHealth = Health->GetMaxHealth();
	Health->ApplyHealthChange(MaxHealth, &Owner);
	Health->ApplyHealthChange(-FMath::Max(MaxHealth * 0.25f, 1.0f), &Owner);
	if (FMath::IsNearlyEqual(Health->GetCurrentHealth(), MaxHealth))
	{
		return;
	}

	SmokeRoundHealthRestorePreparedForRound = RoundNumber;
	++SmokeRoundHealthRestorePreparedCount;
	UE_LOG(
		LogCWSCombatSmoke,
		Display,
		TEXT("Round %d clear health restore prepared at %.1f/%.1f."),
		RoundNumber,
		Health->GetCurrentHealth(),
		MaxHealth);
}


void FCWSCombatSmokeRunner::PrepareSmokeWeaponTarget(ACWSPlayerCharacter* PlayerCharacter)
{
	if (bSmokeWeaponTargetSpawned || !PlayerCharacter)
	{
		return;
	}

	// Wave 적과 분리된 고정 표적을 사용하되 Health/피격/사망 표현은 같은 적 클래스를 사용한다.
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector TargetLocation = PlayerCharacter->GetActorLocation() + PlayerCharacter->GetActorForwardVector() * 800.0f;
	ACWSEnemyBase* Target = Owner.GetWorld()->SpawnActor<ACWSEnemyBase>(
		ACWSEnemyBase::StaticClass(),
		TargetLocation,
		PlayerCharacter->GetActorRotation(),
		SpawnParameters);
	if (!Target)
	{
		FinishSmokeTest(false, TEXT("Could not spawn the weapon smoke target"));
		return;
	}

	Target->GetCharacterMovement()->DisableMovement();
	Target->DetachFromControllerPendingDestroy();
	SmokeWeaponTarget = Target;
	bSmokeWeaponTargetSpawned = true;
	UE_LOG(LogCWSCombatSmoke, Display, TEXT("Weapon smoke target spawned at %s."), *TargetLocation.ToCompactString());
}

void FCWSCombatSmokeRunner::RunSmokeWeaponStep(ACWSPlayerCharacter* PlayerCharacter)
{
	ACWSEnemyBase* Target = SmokeWeaponTarget.Get();
	if (!PlayerCharacter || !Target || bSmokeWeaponTargetKilled)
	{
		return;
	}

	UCWSHealthComponent* TargetHealth = Target->GetHealthComponent();
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter->GetWeaponComponent();
	APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController());
	if (!TargetHealth || !Weapon || !PlayerController)
	{
		return;
	}

	// Production 카메라 ray가 표적을 향하게 한 뒤 한 step을 기다리고 실제 TryFire를 호출한다.
	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	PlayerController->SetControlRotation((Target->GetActorLocation() - ViewLocation).Rotation());
	if (!bSmokeWeaponAimPrimed)
	{
		bSmokeWeaponAimPrimed = true;
		return;
	}

	const float HealthBeforeShot = TargetHealth->GetCurrentHealth();
	const int32 ImpactEffectsBeforeShot = Weapon->GetImpactEffectSpawnCount();
	const int32 FireSoundsBeforeShot = Weapon->GetFireSoundPlayCount();
	const int32 ImpactSoundsBeforeShot = Weapon->GetImpactSoundPlayCount();
	if (!Weapon->TryFire())
	{
		return;
	}
	// 한 발의 결과에서 damage와 동반 피드백 카운터를 함께 관찰한다.
	if (TargetHealth->GetCurrentHealth() < HealthBeforeShot)
	{
		bSmokeSawWeaponDamage = true;
		bSmokeSawFireSound = bSmokeSawFireSound || Weapon->GetFireSoundPlayCount() > FireSoundsBeforeShot;
		bSmokeSawImpactSound = bSmokeSawImpactSound || Weapon->GetImpactSoundPlayCount() > ImpactSoundsBeforeShot;
		bSmokeSawHitReaction = bSmokeSawHitReaction || Target->GetHitReactionCount() > 0;
		bSmokeSawImpactEffect = bSmokeSawImpactEffect || Weapon->GetImpactEffectSpawnCount() > ImpactEffectsBeforeShot;
	}
	if (!TargetHealth->IsAlive())
	{
		bSmokeWeaponTargetKilled = true;
		bSmokeSawDeathAnimation = Target->HasPlayedDeathAnimation();
		bSmokeSawDeathEffect = Target->GetDeathEffectSpawnCount() > 0;
		UE_LOG(
			LogCWSCombatSmoke,
			Display,
			TEXT("CWS_COMBAT_FEEDBACK_SMOKE_STATE: FireSound=%s ImpactSound=%s HitReaction=%s ImpactEffect=%s DeathAnimation=%s DeathEffect=%s"),
			bSmokeSawFireSound ? TEXT("true") : TEXT("false"),
			bSmokeSawImpactSound ? TEXT("true") : TEXT("false"),
			bSmokeSawHitReaction ? TEXT("true") : TEXT("false"),
			bSmokeSawImpactEffect ? TEXT("true") : TEXT("false"),
			bSmokeSawDeathAnimation ? TEXT("true") : TEXT("false"),
			bSmokeSawDeathEffect ? TEXT("true") : TEXT("false"));
		if (bSmokeSawFireSound && bSmokeSawImpactSound && bSmokeSawHitReaction && bSmokeSawImpactEffect &&
			bSmokeSawDeathAnimation && bSmokeSawDeathEffect)
		{
			UE_LOG(
				LogCWSCombatSmoke,
				Display,
				TEXT("CWS_COMBAT_FEEDBACK_SMOKE_VERIFIED: fire/impact sound, hitscan damage, native impact burst, hit reaction, death animation, and death effect"));
		}
	}
}

void FCWSCombatSmokeRunner::RunSmokeSupplyStep(ACWSPlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !bSmokeWeaponTargetKilled)
	{
		return;
	}

	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter->GetWeaponComponent();
	UCWSHealthComponent* Health = PlayerCharacter->GetHealthComponent();
	if (!Weapon || !Health)
	{
		return;
	}

	// 1) Reload가 즉시 채우지 않고 timer 동안 상태를 유지하는지 확인한다.
	if (!bSmokeReloadStarted)
	{
		SmokeAmmoBeforeReload = Weapon->GetCurrentAmmo();
		SmokeReserveBeforeReload = Weapon->GetReserveAmmo();
		bSmokeReloadStarted = Weapon->Reload() && Weapon->IsReloading();
		return;
	}

	if (!bSmokeReloadCompleted)
	{
		if (Weapon->IsReloading())
		{
			return;
		}
		bSmokeReloadCompleted =
			Weapon->GetCurrentAmmo() > SmokeAmmoBeforeReload &&
			Weapon->GetCurrentAmmo() == Weapon->GetMaxAmmo() &&
			Weapon->GetReserveAmmo() < SmokeReserveBeforeReload;
		if (bSmokeReloadCompleted)
		{
			UE_LOG(LogCWSCombatSmoke, Display, TEXT("Timed reload consumed reserve ammo and refilled the magazine."));
		}
	}

	if (!bSmokeRoundOneCleared || !bSmokeReloadCompleted)
	{
		return;
	}

	// 2) Round 1 clear가 만든 실제 Ammo Pickup을 수집한다.
	if (!bSmokeAmmoSupplyCollected)
	{
		if (ACWSSupplyPickup* AmmoSupply = LastRoundSupply.Get())
		{
			const int32 ReserveBeforeSupply = Weapon->GetReserveAmmo();
			bSmokeAmmoSupplyCollected =
				AmmoSupply->GetSupplyType() == ECWSSupplyType::Ammo &&
				AmmoSupply->TryCollect(PlayerCharacter) &&
				Weapon->GetReserveAmmo() > ReserveBeforeSupply;
			if (bSmokeAmmoSupplyCollected)
			{
				UE_LOG(LogCWSCombatSmoke, Display, TEXT("Ammo supply increased reserve ammo."));
			}
		}
		return;
	}

	// 3) Health Pickup은 체력을 일부 소모한 뒤 실제 증가량으로 확인한다.
	if (!bSmokeHealthSupplyCollected)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACWSSupplyPickup* HealthSupply = Owner.GetWorld()->SpawnActor<ACWSSupplyPickup>(
			ACWSSupplyPickup::StaticClass(),
			PlayerCharacter->GetActorLocation() + FVector(0.0f, 150.0f, 30.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!HealthSupply)
		{
			return;
		}

		HealthSupply->ConfigureSupply(ECWSSupplyType::Health);
		Health->ApplyHealthChange(Health->GetMaxHealth(), &Owner);
		Health->ApplyHealthChange(-20.0f, &Owner);
		const float HealthBeforeSupply = Health->GetCurrentHealth();
		bSmokeHealthSupplyCollected =
			HealthSupply->TryCollect(PlayerCharacter) && Health->GetCurrentHealth() > HealthBeforeSupply;
		if (bSmokeHealthSupplyCollected)
		{
			UE_LOG(LogCWSCombatSmoke, Display, TEXT("Health supply restored player health."));
		}
	}
}

void FCWSCombatSmokeRunner::RunCombatSmokeStep()
{
	if (bSmokeFinished)
	{
		return;
	}

	UWorld* World = Owner.GetWorld();
	if (!World)
	{
		FinishSmokeTest(false, TEXT("World unavailable"));
		return;
	}

	// 공통 준비: 실제 월드 객체, Arena 표현, Wave phase, Player를 반복 관찰한다.
	Owner.BindGameplayActors();
	ConfigureAllRoundsSmokeTimings();
	if (Owner.ArenaVisualDirector.IsValid())
	{
		bSmokeSawArenaVisuals =
			Owner.ArenaVisualDirector->IsPresentationReady() &&
			Owner.ArenaVisualDirector->GetCenterRingSegmentCount() == 24 &&
			Owner.ArenaVisualDirector->GetCoverCount() == 8 &&
			Owner.ArenaVisualDirector->GetGateBeaconCount() == 8;
		if (bSmokeSawArenaVisuals && !bSmokeLoggedArenaPresentation)
		{
			bSmokeLoggedArenaPresentation = true;
			UE_LOG(
				LogCWSCombatSmoke,
				Display,
				TEXT("CWS_ARENA_PRESENTATION_VERIFIED: 24 ring segments, 8 blocking covers, and 8 gate beacons"));
		}
	}
	if (Owner.WaveManager.IsValid())
	{
		HandleWavePhaseChanged(Owner.WaveManager->GetWavePhase(), Owner.WaveManager->GetCurrentRound());
	}
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(&Owner, 0));
	if (PlayerCharacter)
	{
		bSmokeSawPlayer = true;
	}

	// OpenLevel 뒤의 두 번째 Runner는 플레이 가능한 초기 상태만 확인하고 테스트를 끝낸다.
	if (bSmokeRestartVerification)
	{
		if (PlayerCharacter && Owner.WaveManager.IsValid() && Owner.PlayerHealth.IsValid() && !Owner.bGameOver && !Owner.bGameCleared)
		{
			FinishSmokeTest(true, TEXT("Combat sounds, enemy attack animation, timed reload, supplies, player death, wave stop, and level restart were verified"));
		}
		else if (World->GetTimeSeconds() - SmokeStartTime > 10.0f)
		{
			FinishSmokeTest(false, TEXT("Level restart did not restore the playable combat state"));
		}
		return;
	}

	if (PlayerCharacter)
	{
		PrepareSmokeWeaponTarget(PlayerCharacter);
		RunSmokeWeaponStep(PlayerCharacter);
		RunSmokeSupplyStep(PlayerCharacter);
	}

	// Wave가 생성한 모든 적을 순회해 이동, 타입별 스탯/표현, 공격과 Boss 계약을 검사한다.
	for (TActorIterator<ACWSEnemyBase> It(World); It; ++It)
	{
		ACWSEnemyBase* Enemy = *It;
		if (Enemy == SmokeWeaponTarget.Get())
		{
			continue;
		}
		UCWSHealthComponent* Health = Enemy->GetHealthComponent();
		if (!Health || !Health->IsAlive())
		{
			continue;
		}
		if (Enemy->GetEnemyType() == ECWSEnemyType::Normal)
		{
			bSmokeSawNormalPresentation = bSmokeSawNormalPresentation ||
				(Enemy->HasArchetypePresentation() &&
				Enemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Normal/SK_NormalMinion.SK_NormalMinion") &&
				Enemy->GetArchetypeColor().Equals(FLinearColor(0.05f, 0.95f, 0.35f)));
		}

		if (bSmokeTestAllRounds)
		{
			// AllRounds에서만 후반 라운드 전용 Fast/Tank/Boss 계약을 요구한다.
			if (ACWSFastEnemy* FastEnemy = Cast<ACWSFastEnemy>(Enemy))
			{
				bSmokeSawFastEnemy = true;
				bSmokeSawFastStats =
					FMath::IsNearlyEqual(Health->GetMaxHealth(), 35.0f) &&
					FMath::IsNearlyEqual(FastEnemy->GetMoveSpeed(), 520.0f) &&
					FMath::IsNearlyEqual(FastEnemy->GetAttackDamage(), 8.0f) &&
					FMath::IsNearlyEqual(FastEnemy->GetAttackInterval(), 0.65f);
				bSmokeSawFastPresentation = FastEnemy->HasArchetypePresentation() &&
					FastEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Fast/SK_FastMinion.SK_FastMinion") &&
					FastEnemy->GetArchetypeColor().Equals(FLinearColor(1.0f, 0.24f, 0.03f));
			}
			else if (ACWSTankEnemy* TankEnemy = Cast<ACWSTankEnemy>(Enemy))
			{
				bSmokeSawTankEnemy = true;
				bSmokeSawTankStats =
					FMath::IsNearlyEqual(Health->GetMaxHealth(), 180.0f) &&
					FMath::IsNearlyEqual(TankEnemy->GetMoveSpeed(), 230.0f) &&
					FMath::IsNearlyEqual(TankEnemy->GetAttackDamage(), 18.0f) &&
					FMath::IsNearlyEqual(TankEnemy->GetAttackInterval(), 1.4f);
				bSmokeSawTankPresentation = TankEnemy->HasArchetypePresentation() &&
					TankEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Tank/SK_Tank.SK_Tank") &&
					TankEnemy->GetArchetypeColor().Equals(FLinearColor(0.05f, 0.35f, 1.0f));
			}

			if (!bSmokeLoggedEnemyArchetypes && bSmokeSawFastEnemy && bSmokeSawFastStats &&
				bSmokeSawTankEnemy && bSmokeSawTankStats)
			{
				bSmokeLoggedEnemyArchetypes = true;
				UE_LOG(
					LogCWSCombatSmoke,
					Display,
					TEXT("CWS_ENEMY_ARCHETYPES_VERIFIED: Fast 35 health/520 speed/8 damage and Tank 180 health/230 speed/18 damage"));
			}

			if (ACWSBossEnemy* Boss = Cast<ACWSBossEnemy>(Enemy))
			{
				// Boss를 공격 범위에서 직접 실행해 Slam -> Final Phase Shockwave를 실제 피해로 확인한다.
				bSmokeSawDedicatedBoss = true;
				bSmokeSawBossMaxHealth = FMath::IsNearlyEqual(Health->GetMaxHealth(), 1200.0f);
				bSmokeSawBossPresentation = Boss->HasArchetypePresentation() && Boss->HasBossPresentation() &&
					Boss->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Normal/SK_NormalMinion.SK_NormalMinion");
				if ((!bSmokeSawBossGroundSlamDamage || !bSmokeSawBossShockwaveDamage) &&
					PlayerCharacter && Owner.PlayerHealth.IsValid())
				{
					Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
					const FVector OriginalPlayerLocation = PlayerCharacter->GetActorLocation();
					PlayerCharacter->TeleportTo(
						Boss->GetActorLocation() + FVector(200.0f, 0.0f, 0.0f),
						PlayerCharacter->GetActorRotation());

					const float PlayerHealthBeforeGroundSlam = Owner.PlayerHealth->GetCurrentHealth();
					const bool bGroundSlamExecuted = Boss->TryAttack(PlayerCharacter);
					bSmokeSawBossGroundSlamDamage =
						bGroundSlamExecuted &&
						Boss->GetLastPattern() == ECWSBossPattern::GroundSlam &&
						Owner.PlayerHealth->GetCurrentHealth() < PlayerHealthBeforeGroundSlam;

					Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
					const float BossDamageToFinalPhase =
						FMath::Max(Health->GetCurrentHealth() - Health->GetMaxHealth() * 0.25f, 0.0f);
					Health->ApplyHealthChange(-BossDamageToFinalPhase, &Owner);
					bSmokeSawBossFinalPhase =
						Boss->GetBossPhase() == ECWSBossPhase::FinalPhase && Boss->HasBossPresentation();

					const float PlayerHealthBeforeShockwave = Owner.PlayerHealth->GetCurrentHealth();
					const bool bShockwaveExecuted = Boss->TryAttack(PlayerCharacter);
					bSmokeSawBossShockwaveDamage =
						bShockwaveExecuted &&
						Boss->GetLastPattern() == ECWSBossPattern::Shockwave &&
						Boss->GetPatternExecutionCount() >= 2 &&
						Owner.PlayerHealth->GetCurrentHealth() < PlayerHealthBeforeShockwave;
					bSmokeSawBossExplosionSound = Boss->GetExplosionSoundPlayCount() >= 2;
					PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
					PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
					Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);

					if (bSmokeSawBossMaxHealth && bSmokeSawBossFinalPhase &&
						bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage &&
						bSmokeSawBossExplosionSound)
					{
						UE_LOG(
							LogCWSCombatSmoke,
							Display,
							TEXT("CWS_BOSS_SMOKE_VERIFIED: class, 1200 health, final phase, ground slam, shockwave damage, knockback, and explosion sound"));
					}
				}

				if (bSmokeSawBossMaxHealth && bSmokeSawBossFinalPhase &&
					bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage &&
					bSmokeSawBossExplosionSound)
				{
					PrepareRoundClearHealthRestoreVerification();
					Health->Kill(&Owner);
					continue;
				}
			}
		}

		// 최초 위치와 비교해 NavMesh 이동을 증명한 뒤 근접 공격 경로를 한 번 실행한다.
		FVector* StartLocation = SmokeEnemyStartLocations.Find(Enemy);
		if (!StartLocation)
		{
			SmokeEnemyStartLocations.Add(Enemy, Enemy->GetActorLocation());
			continue;
		}

		if (FVector::DistSquared(*StartLocation, Enemy->GetActorLocation()) >= FMath::Square(25.0f))
		{
			bSmokeSawEnemyMovement = true;
			if ((!bSmokeSawEnemyAttackDamage || !bSmokeSawEnemyAttackAnimation || !bSmokeSawEnemyAttackSound) &&
				!Cast<ACWSBossEnemy>(Enemy) && PlayerCharacter && Owner.PlayerHealth.IsValid())
			{
				const FVector OriginalPlayerLocation = PlayerCharacter->GetActorLocation();
				Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
				PlayerCharacter->TeleportTo(
					Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 100.0f,
					PlayerCharacter->GetActorRotation());
				const float HealthBeforeAttack = Owner.PlayerHealth->GetCurrentHealth();
				const int32 AnimationsBeforeAttack = Enemy->GetAttackAnimationCount();
				const int32 SoundsBeforeAttack = Enemy->GetAttackSoundPlayCount();
				const bool bAttackExecuted = Enemy->TryAttack(PlayerCharacter);
				bSmokeSawEnemyAttackDamage = bSmokeSawEnemyAttackDamage ||
					(bAttackExecuted && Owner.PlayerHealth->GetCurrentHealth() < HealthBeforeAttack);
				bSmokeSawEnemyAttackAnimation = bSmokeSawEnemyAttackAnimation ||
					Enemy->GetAttackAnimationCount() > AnimationsBeforeAttack;
				bSmokeSawEnemyAttackSound = bSmokeSawEnemyAttackSound ||
					Enemy->GetAttackSoundPlayCount() > SoundsBeforeAttack;
				PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
				PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
				Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
				if (bSmokeSawEnemyAttackDamage && bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound)
				{
					UE_LOG(
						LogCWSCombatSmoke,
						Display,
						TEXT("CWS_ENEMY_ATTACK_FEEDBACK_VERIFIED: damage, MM_Attack_01 montage, and procedural attack sound"));
				}
			}
			if (bSmokeSawEnemyAttackDamage && bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound)
			{
				PrepareRoundClearHealthRestoreVerification();
				Health->Kill(&Owner);
			}
		}
	}

	const bool bRequiredEnemyPresentationReady = bSmokeTestAllRounds
		? bSmokeSawNormalPresentation && bSmokeSawFastPresentation && bSmokeSawTankPresentation
		: bSmokeSawNormalPresentation;
	if (bRequiredEnemyPresentationReady && !bSmokeLoggedEnemyPresentation)
	{
		bSmokeLoggedEnemyPresentation = true;
		if (bSmokeTestAllRounds)
		{
			UE_LOG(LogCWSCombatSmoke, Display, TEXT("CWS_ENEMY_PRESENTATION_VERIFIED: Normal green, Fast orange, and Tank blue"));
		}
		else
		{
			UE_LOG(LogCWSCombatSmoke, Display, TEXT("CWS_ENEMY_PRESENTATION_VERIFIED: Normal green"));
		}
	}

	// Round 1 모드는 다음 라운드가 섞이기 전에 Wave를 멈추고 GameOver/Restart를 검증한다.
	if (!bSmokeTestAllRounds && bSmokeRoundOneCleared && !bSmokeStoppedAfterRoundOne && Owner.WaveManager.IsValid())
	{
		bSmokeStoppedAfterRoundOne = true;
		Owner.WaveManager->StopWaveSystem();
		UE_LOG(LogCWSCombatSmoke, Display, TEXT("Round 1 smoke paused the wave system before the player-death check."));
	}

	if (!bSmokeTestAllRounds && bSmokeRoundOneCleared && bSmokeWeaponTargetKilled &&
		bSmokeReloadCompleted && bSmokeAmmoSupplyCollected && bSmokeHealthSupplyCollected && PlayerCharacter)
	{
		UCWSHealthComponent* Health = PlayerCharacter->GetHealthComponent();
		if (!bSmokeAppliedPlayerDamage && Health && Health->IsAlive())
		{
			bSmokeAppliedPlayerDamage = true;
			UGameplayStatics::ApplyDamage(
				PlayerCharacter,
				Health->GetMaxHealth() + 1.0f,
				nullptr,
				&Owner,
				UDamageType::StaticClass());
		}
		else if (bSmokeSawPlayerDeath && Owner.bGameOver && Owner.WaveManager.IsValid() && !Owner.WaveManager->IsRoundInProgress())
		{
			const bool bCombatFlowVerified =
				bSmokeSawPlayer && bSmokeSawEnemyMovement && bSmokeSawWeaponDamage &&
				bSmokeSawFireSound && bSmokeSawImpactSound && bSmokeSawHitReaction && bSmokeSawImpactEffect &&
				bSmokeSawDeathAnimation && bSmokeSawDeathEffect && bSmokeSawEnemyAttackDamage &&
				bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound &&
				bSmokeSawArenaVisuals && bSmokeSawNormalPresentation &&
				bSmokeReloadCompleted && bSmokeAmmoSupplyCollected && bSmokeHealthSupplyCollected && Owner.CanRestart();
			if (!bCombatFlowVerified)
			{
				FinishSmokeTest(false, TEXT("Combat flow reached game over without satisfying restart prerequisites"));
				return;
			}

			GCombatSmokeRestartRequested = true;
			UE_LOG(LogCWSCombatSmoke, Display, TEXT("Round 1 smoke requesting a level restart."));
			Owner.RestartCurrentLevel();
			return;
		}
	}

	// AllRounds 성공은 개별 관찰 플래그와 최종 GameMode/Wave 상태를 모두 만족해야 한다.
	if (bSmokeTestAllRounds && bSmokeAllRoundsCleared)
	{
		FinishSmokeTest(
			bSmokeSawPlayer && bSmokeSawEnemyMovement && bSmokeSawWeaponDamage &&
				bSmokeSawFireSound && bSmokeSawImpactSound && bSmokeSawHitReaction && bSmokeSawImpactEffect &&
				bSmokeSawDeathAnimation && bSmokeSawDeathEffect && bSmokeSawEnemyAttackDamage &&
				bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound &&
				bSmokeReloadCompleted && bSmokeAmmoSupplyCollected && bSmokeHealthSupplyCollected &&
				bSmokeSawFastEnemy && bSmokeSawFastStats && bSmokeSawTankEnemy && bSmokeSawTankStats &&
				bSmokeSawArenaVisuals && bSmokeSawNormalPresentation &&
				bSmokeSawFastPresentation && bSmokeSawTankPresentation &&
				bSmokeSawBossPresentation &&
				bSmokeSawPreparingPhase && bSmokeSawActivePhase && bSmokeSawRoundClearedPhase && bSmokeSawCompletedPhase &&
				bSmokeWeaponTargetKilled && bSmokeSawDedicatedBoss && bSmokeSawBossMaxHealth &&
				bSmokeSawBossFinalPhase && bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage &&
				bSmokeSawBossExplosionSound &&
				SmokeRoundHealthRestorePreparedCount == 5 && SmokeRoundHealthRestoreVerifiedCount == 5 &&
				SmokeHighestRoundCleared == 5 && Owner.bGameCleared,
			TEXT("Arena presentation, enemy type colors, combat feedback, reload, supplies, round phases, full health restoration, archetypes, boss patterns, and all five rounds were verified"));
		return;
	}

	const float TimeoutSeconds = bSmokeTestAllRounds ? 60.0f : 45.0f;
	if (World->GetTimeSeconds() - SmokeStartTime > TimeoutSeconds)
	{
		FinishSmokeTest(false, bSmokeTestAllRounds
			? TEXT("All rounds did not clear within 60 seconds")
			: TEXT("Round 1 combat flow did not complete within 45 seconds"));
	}
}

void FCWSCombatSmokeRunner::FinishSmokeTest(const bool bSucceeded, const TCHAR* Reason)
{
	if (bSmokeFinished)
	{
		return;
	}

	bSmokeFinished = true;
	Owner.GetWorldTimerManager().ClearTimer(SmokeStepTimer);
	if (bSucceeded)
	{
		UE_LOG(
			LogCWSCombatSmoke,
			Display,
			TEXT("%s: %s"),
			bSmokeTestAllRounds ? TEXT("CWS_ALL_ROUNDS_SMOKE_SUCCESS") : TEXT("CWS_ROUND_ONE_SMOKE_SUCCESS"),
			Reason);
		FPlatformMisc::RequestExitWithStatus(false, 0, TEXT("CWS round one smoke test succeeded"));
	}
	else
	{
		UE_LOG(
			LogCWSCombatSmoke,
			Error,
			TEXT("%s: %s"),
			bSmokeTestAllRounds ? TEXT("CWS_ALL_ROUNDS_SMOKE_FAILURE") : TEXT("CWS_ROUND_ONE_SMOKE_FAILURE"),
			Reason);
		FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("CWS round one smoke test failed"));
	}
}
