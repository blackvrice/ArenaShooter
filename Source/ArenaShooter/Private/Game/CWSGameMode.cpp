#include "Game/CWSGameMode.h"

#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Enemy/CWSBossEnemy.h"
#include "Enemy/CWSEnemyBase.h"
#include "Enemy/CWSFastEnemy.h"
#include "Enemy/CWSTankEnemy.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CWSPlayerCharacter.h"
#include "Pickup/CWSSupplyPickup.h"
#include "TimerManager.h"
#include "UI/CWSHUD.h"
#include "Wave/CWSWaveManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSGame, Log, All);

namespace
{
	bool GCombatSmokeRestartRequested = false;
}

ACWSGameMode::ACWSGameMode()
{
	DefaultPawnClass = ACWSPlayerCharacter::StaticClass();
	HUDClass = ACWSHUD::StaticClass();
}

void ACWSGameMode::BeginPlay()
{
	Super::BeginPlay();

	BindGameplayActors();
	GetWorldTimerManager().SetTimer(
		GameplayBindTimer,
		this,
		&ACWSGameMode::BindGameplayActors,
		0.1f,
		true,
		0.1f);

	bSmokeTestAllRounds = FParse::Param(FCommandLine::Get(), TEXT("CWSAllRoundsSmokeTest"));
	bSmokeTestEnabled = bSmokeTestAllRounds || FParse::Param(FCommandLine::Get(), TEXT("CWSRoundOneSmokeTest"));
	if (!bSmokeTestEnabled)
	{
		return;
	}

	SmokeStartTime = GetWorld()->GetTimeSeconds();
	bSmokeRestartVerification = !bSmokeTestAllRounds && GCombatSmokeRestartRequested;
	GetWorldTimerManager().SetTimer(
		SmokeStepTimer,
		this,
		&ACWSGameMode::RunCombatSmokeStep,
		0.1f,
		true,
		0.1f);
	UE_LOG(
		LogCWSGame,
		Display,
		TEXT("%s"),
		bSmokeTestAllRounds
			? TEXT("CWS_ALL_ROUNDS_SMOKE_STARTED")
			: bSmokeRestartVerification
				? TEXT("CWS_ROUND_ONE_RESTART_VALIDATION_STARTED")
				: TEXT("CWS_ROUND_ONE_SMOKE_STARTED"));
}

void ACWSGameMode::BindGameplayActors()
{
	if (!WaveManager.IsValid())
	{
		for (TActorIterator<ACWSWaveManager> It(GetWorld()); It; ++It)
		{
			WaveManager = *It;
			It->OnRoundCleared.AddUniqueDynamic(this, &ACWSGameMode::HandleRoundCleared);
			It->OnAllRoundsCompleted.AddUniqueDynamic(this, &ACWSGameMode::HandleAllRoundsCompleted);
			break;
		}
	}

	if (!PlayerHealth.IsValid())
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (UCWSHealthComponent* Health = PlayerPawn->FindComponentByClass<UCWSHealthComponent>())
			{
				PlayerHealth = Health;
				Health->OnDeath.AddUniqueDynamic(this, &ACWSGameMode::HandlePlayerDeath);
			}
		}
	}

	if (bSmokeTestAllRounds)
	{
		ConfigureAllRoundsSmokeTimings();
	}

	if (WaveManager.IsValid() && PlayerHealth.IsValid())
	{
		GetWorldTimerManager().ClearTimer(GameplayBindTimer);
	}
}

void ACWSGameMode::ConfigureAllRoundsSmokeTimings()
{
	if (bSmokeTimingsConfigured || !WaveManager.IsValid())
	{
		return;
	}

	for (FCWSRoundDefinition& Round : WaveManager->Rounds)
	{
		Round.PreRoundDelay = 0.05f;
		Round.PostRoundDelay = 0.05f;
		for (FCWSRoundSpawnGroup& Group : Round.SpawnGroups)
		{
			Group.SpawnInterval = 0.05f;
		}
	}
	bSmokeTimingsConfigured = true;
	UE_LOG(LogCWSGame, Display, TEXT("All-round smoke timings accelerated."));
}

void ACWSGameMode::PrepareSmokeWeaponTarget(ACWSPlayerCharacter* PlayerCharacter)
{
	if (bSmokeWeaponTargetSpawned || !PlayerCharacter)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector TargetLocation = PlayerCharacter->GetActorLocation() + PlayerCharacter->GetActorForwardVector() * 800.0f;
	ACWSEnemyBase* Target = GetWorld()->SpawnActor<ACWSEnemyBase>(
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
	UE_LOG(LogCWSGame, Display, TEXT("Weapon smoke target spawned at %s."), *TargetLocation.ToCompactString());
}

void ACWSGameMode::RunSmokeWeaponStep(ACWSPlayerCharacter* PlayerCharacter)
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
	if (!Weapon->TryFire())
	{
		return;
	}
	if (TargetHealth->GetCurrentHealth() < HealthBeforeShot)
	{
		bSmokeSawWeaponDamage = true;
	}
	if (!TargetHealth->IsAlive())
	{
		bSmokeWeaponTargetKilled = true;
		UE_LOG(LogCWSGame, Display, TEXT("Weapon smoke target killed through hitscan damage."));
	}
}

void ACWSGameMode::RunSmokeSupplyStep(ACWSPlayerCharacter* PlayerCharacter)
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
			UE_LOG(LogCWSGame, Display, TEXT("Timed reload consumed reserve ammo and refilled the magazine."));
		}
	}

	if (!bSmokeRoundOneCleared || !bSmokeReloadCompleted)
	{
		return;
	}

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
				UE_LOG(LogCWSGame, Display, TEXT("Ammo supply increased reserve ammo."));
			}
		}
		return;
	}

	if (!bSmokeHealthSupplyCollected)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACWSSupplyPickup* HealthSupply = GetWorld()->SpawnActor<ACWSSupplyPickup>(
			ACWSSupplyPickup::StaticClass(),
			PlayerCharacter->GetActorLocation() + FVector(0.0f, 150.0f, 30.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!HealthSupply)
		{
			return;
		}

		HealthSupply->ConfigureSupply(ECWSSupplyType::Health);
		Health->ApplyHealthChange(Health->GetMaxHealth(), this);
		Health->ApplyHealthChange(-20.0f, this);
		const float HealthBeforeSupply = Health->GetCurrentHealth();
		bSmokeHealthSupplyCollected =
			HealthSupply->TryCollect(PlayerCharacter) && Health->GetCurrentHealth() > HealthBeforeSupply;
		if (bSmokeHealthSupplyCollected)
		{
			UE_LOG(LogCWSGame, Display, TEXT("Health supply restored player health."));
		}
	}
}

void ACWSGameMode::RunCombatSmokeStep()
{
	if (bSmokeFinished)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishSmokeTest(false, TEXT("World unavailable"));
		return;
	}

	BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (PlayerCharacter)
	{
		bSmokeSawPlayer = true;
	}

	if (bSmokeRestartVerification)
	{
		if (PlayerCharacter && WaveManager.IsValid() && PlayerHealth.IsValid() && !bGameOver && !bGameCleared)
		{
			FinishSmokeTest(true, TEXT("Hitscan combat, timed reload, supplies, player death, wave stop, and level restart were verified"));
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

		if (bSmokeTestAllRounds)
		{
			if (ACWSFastEnemy* FastEnemy = Cast<ACWSFastEnemy>(Enemy))
			{
				bSmokeSawFastEnemy = true;
				bSmokeSawFastStats =
					FMath::IsNearlyEqual(Health->GetMaxHealth(), 35.0f) &&
					FMath::IsNearlyEqual(FastEnemy->GetMoveSpeed(), 520.0f) &&
					FMath::IsNearlyEqual(FastEnemy->GetAttackDamage(), 8.0f) &&
					FMath::IsNearlyEqual(FastEnemy->GetAttackInterval(), 0.65f);
			}
			else if (ACWSTankEnemy* TankEnemy = Cast<ACWSTankEnemy>(Enemy))
			{
				bSmokeSawTankEnemy = true;
				bSmokeSawTankStats =
					FMath::IsNearlyEqual(Health->GetMaxHealth(), 180.0f) &&
					FMath::IsNearlyEqual(TankEnemy->GetMoveSpeed(), 230.0f) &&
					FMath::IsNearlyEqual(TankEnemy->GetAttackDamage(), 18.0f) &&
					FMath::IsNearlyEqual(TankEnemy->GetAttackInterval(), 1.4f);
			}

			if (!bSmokeLoggedEnemyArchetypes && bSmokeSawFastEnemy && bSmokeSawFastStats &&
				bSmokeSawTankEnemy && bSmokeSawTankStats)
			{
				bSmokeLoggedEnemyArchetypes = true;
				UE_LOG(
					LogCWSGame,
					Display,
					TEXT("CWS_ENEMY_ARCHETYPES_VERIFIED: Fast 35 health/520 speed/8 damage and Tank 180 health/230 speed/18 damage"));
			}

			if (ACWSBossEnemy* Boss = Cast<ACWSBossEnemy>(Enemy))
			{
				bSmokeSawDedicatedBoss = true;
				bSmokeSawBossMaxHealth = FMath::IsNearlyEqual(Health->GetMaxHealth(), 1200.0f);
				if ((!bSmokeSawBossGroundSlamDamage || !bSmokeSawBossShockwaveDamage) &&
					PlayerCharacter && PlayerHealth.IsValid())
				{
					PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
					const FVector OriginalPlayerLocation = PlayerCharacter->GetActorLocation();
					PlayerCharacter->TeleportTo(
						Boss->GetActorLocation() + FVector(200.0f, 0.0f, 0.0f),
						PlayerCharacter->GetActorRotation());

					const float PlayerHealthBeforeGroundSlam = PlayerHealth->GetCurrentHealth();
					const bool bGroundSlamExecuted = Boss->TryAttack(PlayerCharacter);
					bSmokeSawBossGroundSlamDamage =
						bGroundSlamExecuted &&
						Boss->GetLastPattern() == ECWSBossPattern::GroundSlam &&
						PlayerHealth->GetCurrentHealth() < PlayerHealthBeforeGroundSlam;

					PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
					const float BossDamageToFinalPhase =
						FMath::Max(Health->GetCurrentHealth() - Health->GetMaxHealth() * 0.25f, 0.0f);
					Health->ApplyHealthChange(-BossDamageToFinalPhase, this);
					bSmokeSawBossFinalPhase = Boss->GetBossPhase() == ECWSBossPhase::FinalPhase;

					const float PlayerHealthBeforeShockwave = PlayerHealth->GetCurrentHealth();
					const bool bShockwaveExecuted = Boss->TryAttack(PlayerCharacter);
					bSmokeSawBossShockwaveDamage =
						bShockwaveExecuted &&
						Boss->GetLastPattern() == ECWSBossPattern::Shockwave &&
						Boss->GetPatternExecutionCount() >= 2 &&
						PlayerHealth->GetCurrentHealth() < PlayerHealthBeforeShockwave;
					PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
					PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
					PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);

					if (bSmokeSawBossMaxHealth && bSmokeSawBossFinalPhase &&
						bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage)
					{
						UE_LOG(
							LogCWSGame,
							Display,
							TEXT("CWS_BOSS_SMOKE_VERIFIED: class, 1200 health, final phase, ground slam, shockwave damage, and knockback path"));
					}
				}

				if (bSmokeSawBossMaxHealth && bSmokeSawBossFinalPhase &&
					bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage)
				{
					Health->Kill(this);
					continue;
				}
			}
		}

		FVector* StartLocation = SmokeEnemyStartLocations.Find(Enemy);
		if (!StartLocation)
		{
			SmokeEnemyStartLocations.Add(Enemy, Enemy->GetActorLocation());
			continue;
		}

		if (FVector::DistSquared(*StartLocation, Enemy->GetActorLocation()) >= FMath::Square(25.0f))
		{
			bSmokeSawEnemyMovement = true;
			Health->Kill(this);
		}
	}

	if (!bSmokeTestAllRounds && bSmokeRoundOneCleared && !bSmokeStoppedAfterRoundOne && WaveManager.IsValid())
	{
		bSmokeStoppedAfterRoundOne = true;
		WaveManager->StopWaveSystem();
		UE_LOG(LogCWSGame, Display, TEXT("Round 1 smoke paused the wave system before the player-death check."));
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
				this,
				UDamageType::StaticClass());
		}
		else if (bSmokeSawPlayerDeath && bGameOver && WaveManager.IsValid() && !WaveManager->IsRoundInProgress())
		{
			const bool bCombatFlowVerified =
				bSmokeSawPlayer && bSmokeSawEnemyMovement && bSmokeSawWeaponDamage &&
				bSmokeReloadCompleted && bSmokeAmmoSupplyCollected && bSmokeHealthSupplyCollected && CanRestart();
			if (!bCombatFlowVerified)
			{
				FinishSmokeTest(false, TEXT("Combat flow reached game over without satisfying restart prerequisites"));
				return;
			}

			GCombatSmokeRestartRequested = true;
			UE_LOG(LogCWSGame, Display, TEXT("Round 1 smoke requesting a level restart."));
			RestartCurrentLevel();
			return;
		}
	}

	if (bSmokeTestAllRounds && bSmokeAllRoundsCleared)
	{
		FinishSmokeTest(
			bSmokeSawPlayer && bSmokeSawEnemyMovement && bSmokeSawWeaponDamage &&
				bSmokeReloadCompleted && bSmokeAmmoSupplyCollected && bSmokeHealthSupplyCollected &&
				bSmokeSawFastEnemy && bSmokeSawFastStats && bSmokeSawTankEnemy && bSmokeSawTankStats &&
				bSmokeWeaponTargetKilled && bSmokeSawDedicatedBoss && bSmokeSawBossMaxHealth &&
				bSmokeSawBossFinalPhase && bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage &&
				SmokeHighestRoundCleared == 5 && bGameCleared,
			TEXT("Hitscan damage, timed reload, supplies, Fast/Tank archetypes, dedicated boss patterns, and all five rounds were verified"));
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

void ACWSGameMode::HandleRoundCleared(const int32 RoundNumber)
{
	SpawnRoundClearSupply(RoundNumber);
	if (!bSmokeTestEnabled)
	{
		return;
	}
	SmokeHighestRoundCleared = FMath::Max(SmokeHighestRoundCleared, RoundNumber);
	UE_LOG(LogCWSGame, Display, TEXT("Game mode observed round %d clear."), RoundNumber);
	if (RoundNumber == 1)
	{
		bSmokeRoundOneCleared = true;
	}
}

void ACWSGameMode::SpawnRoundClearSupply(const int32 RoundNumber)
{
	if (RoundNumber < 1 || RoundNumber >= 5 || bGameOver || !GetWorld())
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector SpawnLocation =
		PlayerPawn->GetActorLocation() + PlayerPawn->GetActorForwardVector() * 250.0f + FVector(0.0f, 0.0f, 30.0f);
	ACWSSupplyPickup* Supply = GetWorld()->SpawnActor<ACWSSupplyPickup>(
		ACWSSupplyPickup::StaticClass(),
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!Supply)
	{
		return;
	}

	const ECWSSupplyType SupplyType = RoundNumber % 2 == 1 ? ECWSSupplyType::Ammo : ECWSSupplyType::Health;
	Supply->ConfigureSupply(SupplyType);
	LastRoundSupply = Supply;
	OnSupplySpawned.Broadcast(Supply);
	UE_LOG(
		LogCWSGame,
		Display,
		TEXT("Round %d clear spawned a %s supply."),
		RoundNumber,
		SupplyType == ECWSSupplyType::Ammo ? TEXT("ammo") : TEXT("health"));
}

void ACWSGameMode::HandleAllRoundsCompleted()
{
	if (!bGameOver)
	{
		bGameCleared = true;
		OnGameCleared.Broadcast();
		UE_LOG(LogCWSGame, Display, TEXT("Game cleared."));
	}
	if (bSmokeTestAllRounds)
	{
		bSmokeAllRoundsCleared = true;
	}
}

void ACWSGameMode::HandlePlayerDeath(AActor* DeadActor)
{
	if (bGameCleared || bGameOver)
	{
		return;
	}

	bGameOver = true;
	if (WaveManager.IsValid())
	{
		WaveManager->StopWaveSystem();
	}
	OnGameOver.Broadcast();
	UE_LOG(LogCWSGame, Display, TEXT("Game over: player died and the wave system stopped."));
	if (bSmokeTestEnabled && bSmokeAppliedPlayerDamage)
	{
		bSmokeSawPlayerDeath = true;
	}
}

void ACWSGameMode::RestartCurrentLevel()
{
	if (!CanRestart() || !GetWorld())
	{
		return;
	}
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!LevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*LevelName), false);
	}
}

void ACWSGameMode::FinishSmokeTest(const bool bSucceeded, const TCHAR* Reason)
{
	if (bSmokeFinished)
	{
		return;
	}

	bSmokeFinished = true;
	GetWorldTimerManager().ClearTimer(SmokeStepTimer);
	if (bSucceeded)
	{
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("%s: %s"),
			bSmokeTestAllRounds ? TEXT("CWS_ALL_ROUNDS_SMOKE_SUCCESS") : TEXT("CWS_ROUND_ONE_SMOKE_SUCCESS"),
			Reason);
		FPlatformMisc::RequestExitWithStatus(false, 0, TEXT("CWS round one smoke test succeeded"));
	}
	else
	{
		UE_LOG(
			LogCWSGame,
			Error,
			TEXT("%s: %s"),
			bSmokeTestAllRounds ? TEXT("CWS_ALL_ROUNDS_SMOKE_FAILURE") : TEXT("CWS_ROUND_ONE_SMOKE_FAILURE"),
			Reason);
		FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("CWS round one smoke test failed"));
	}
}
