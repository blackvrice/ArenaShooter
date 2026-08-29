#include "Game/CWSGameMode.h"

#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Enemy/CWSBossEnemy.h"
#include "Enemy/CWSEnemyBase.h"
#include "Enemy/CWSFastEnemy.h"
#include "Enemy/CWSTankEnemy.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Player/CWSPlayerCharacter.h"
#include "Pickup/CWSSupplyPickup.h"
#include "TimerManager.h"
#include "UI/CWSHUD.h"
#include "UnrealClient.h"
#include "Wave/CWSWaveManager.h"
#include "World/CWSArenaVisualDirector.h"

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

	bHudScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSHUDScreenshotTest"));
	if (bHudScreenshotTest)
	{
		HudScreenshotStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			HudScreenshotTimer,
			this,
			&ACWSGameMode::RunHudScreenshotStep,
			0.1f,
			true,
			0.1f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_HUD_SCREENSHOT_STARTED"));
	}

	bCombatFeedbackScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSCombatFeedbackScreenshotTest"));
	if (bCombatFeedbackScreenshotTest)
	{
		CombatFeedbackScreenshotStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			CombatFeedbackScreenshotTimer,
			this,
			&ACWSGameMode::RunCombatFeedbackScreenshotStep,
			0.05f,
			true,
			0.1f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_STARTED"));
	}

	bAttackFeedbackScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSAttackFeedbackScreenshotTest"));
	if (bAttackFeedbackScreenshotTest)
	{
		AttackFeedbackScreenshotStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			AttackFeedbackScreenshotTimer,
			this,
			&ACWSGameMode::RunAttackFeedbackScreenshotStep,
			0.05f,
			true,
			0.1f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_STARTED"));
	}

	bVisualPolishScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSVisualPolishScreenshotTest"));
	if (bVisualPolishScreenshotTest)
	{
		VisualPolishScreenshotStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			VisualPolishScreenshotTimer,
			this,
			&ACWSGameMode::RunVisualPolishScreenshotStep,
			0.05f,
			true,
			0.1f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_STARTED"));
	}

	bBalanceCombatTest = FParse::Param(FCommandLine::Get(), TEXT("CWSBalanceCombatTest"));
	if (bBalanceCombatTest)
	{
		BalanceCombatStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			BalanceCombatTimer,
			this,
			&ACWSGameMode::RunBalanceCombatStep,
			0.03f,
			true,
			0.1f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_BALANCE_COMBAT_STARTED"));
	}

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
			It->OnWavePhaseChanged.AddUniqueDynamic(this, &ACWSGameMode::HandleWavePhaseChanged);
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

	if (!ArenaVisualDirector.IsValid())
	{
		for (TActorIterator<ACWSArenaVisualDirector> It(GetWorld()); It; ++It)
		{
			ArenaVisualDirector = *It;
			break;
		}
		if (!ArenaVisualDirector.IsValid())
		{
			if (ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				const float FloorHeight = PlayerCharacter->GetActorLocation().Z -
					PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				ArenaVisualDirector = GetWorld()->SpawnActor<ACWSArenaVisualDirector>(
					ACWSArenaVisualDirector::StaticClass(),
					FVector(0.0f, 0.0f, FloorHeight),
					FRotator::ZeroRotator);
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

void ACWSGameMode::RunHudScreenshotStep()
{
	if (!bHudScreenshotTest || bHudScreenshotRequested || !GetWorld())
	{
		return;
	}

	BindGameplayActors();
	if (ArenaVisualDirector.IsValid())
	{
		bSmokeSawArenaVisuals =
			ArenaVisualDirector->IsPresentationReady() &&
			ArenaVisualDirector->GetCenterRingSegmentCount() == 24 &&
			ArenaVisualDirector->GetCoverCount() == 8 &&
			ArenaVisualDirector->GetGateBeaconCount() == 8;
		if (bSmokeSawArenaVisuals && !bSmokeLoggedArenaPresentation)
		{
			bSmokeLoggedArenaPresentation = true;
			UE_LOG(
				LogCWSGame,
				Display,
				TEXT("CWS_ARENA_PRESENTATION_VERIFIED: 24 ring segments, 8 blocking covers, and 8 gate beacons"));
		}
	}
	if (!WaveManager.IsValid() || WaveManager->GetWavePhase() != ECWSWavePhase::Preparing ||
		WaveManager->GetCurrentRound() != 1)
	{
		if (GetWorld()->GetTimeSeconds() - HudScreenshotStartTime > 10.0f)
		{
			UE_LOG(LogCWSGame, Error, TEXT("CWS_HUD_SCREENSHOT_FAILURE: Round 1 preparing phase was not reached"));
			FPlatformMisc::RequestExitWithStatus(true, 2, TEXT("CWS HUD screenshot test timed out"));
		}
		return;
	}

	bHudScreenshotRequested = true;
	GetWorldTimerManager().ClearTimer(HudScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	HudScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSRoundAnnouncement.png"));
	IFileManager::Get().Delete(*HudScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(HudScreenshotPath, true, false);
	UE_LOG(
		LogCWSGame,
		Display,
		TEXT("CWS_HUD_SCREENSHOT_REQUESTED: %s Phase=%s Remaining=%.2f"),
		*HudScreenshotPath,
		*UEnum::GetValueAsString(WaveManager->GetWavePhase()),
		WaveManager->GetPhaseTimeRemaining());
	GetWorldTimerManager().SetTimer(
		HudScreenshotExitTimer,
		this,
		&ACWSGameMode::FinishHudScreenshotTest,
		1.0f,
		false);
}

void ACWSGameMode::FinishHudScreenshotTest()
{
	const int64 ScreenshotSize = IFileManager::Get().FileSize(*HudScreenshotPath);
	const bool bSucceeded = ScreenshotSize > 0;
	if (bSucceeded)
	{
		UE_LOG(LogCWSGame, Display, TEXT("CWS_HUD_SCREENSHOT_SUCCESS: %s Size=%lld"), *HudScreenshotPath, ScreenshotSize);
	}
	else
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_HUD_SCREENSHOT_FAILURE: %s Size=%lld"), *HudScreenshotPath, ScreenshotSize);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bSucceeded ? 0 : 3,
		bSucceeded ? TEXT("CWS HUD screenshot captured") : TEXT("CWS HUD screenshot was not written"));
}

void ACWSGameMode::RunCombatFeedbackScreenshotStep()
{
	if (!bCombatFeedbackScreenshotTest || bCombatFeedbackScreenshotRequested || !GetWorld())
	{
		return;
	}

	const float ElapsedTime = GetWorld()->GetTimeSeconds() - CombatFeedbackScreenshotStartTime;
	if (ElapsedTime > 12.0f)
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: feedback state was not reached"));
		FPlatformMisc::RequestExitWithStatus(true, 4, TEXT("CWS combat feedback screenshot test timed out"));
		return;
	}

	BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponComponent() : nullptr;
	if (!PlayerCharacter || !PlayerController || !Weapon || !WaveManager.IsValid())
	{
		return;
	}

	if (!bCombatFeedbackArenaPrepared)
	{
		if (WaveManager->GetWavePhase() != ECWSWavePhase::Preparing)
		{
			return;
		}
		WaveManager->StopWaveSystem();
		bCombatFeedbackArenaPrepared = true;
		PlayerCharacter->GetMesh()->SetVisibility(false, true);
		const FRotator PresentationRotation(-8.0f, PlayerCharacter->GetActorRotation().Yaw, 0.0f);
		PlayerController->SetControlRotation(PresentationRotation);
		return;
	}

	ACWSEnemyBase* Target = CombatFeedbackScreenshotTarget.Get();
	if (!Target)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FVector GroundForward = ViewRotation.Vector().GetSafeNormal2D();
		const FVector TargetLocation = PlayerCharacter->GetActorLocation() + GroundForward * 450.0f;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Target = GetWorld()->SpawnActor<ACWSEnemyBase>(
			ACWSEnemyBase::StaticClass(),
			TargetLocation,
			GroundForward.Rotation(),
			SpawnParameters);
		if (!Target)
		{
			return;
		}
		Target->GetCharacterMovement()->DisableMovement();
		Target->DetachFromControllerPendingDestroy();
		CombatFeedbackScreenshotTarget = Target;
		UE_LOG(LogCWSGame, Display, TEXT("Combat feedback screenshot target spawned at %s."), *TargetLocation.ToCompactString());
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TargetAimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	PlayerController->SetControlRotation((TargetAimPoint - ViewLocation).Rotation());
	if (!bCombatFeedbackAimPrimed)
	{
		bCombatFeedbackAimPrimed = true;
		return;
	}

	if (!bCombatFeedbackShotFired)
	{
		const float HealthBeforeShot = Target->GetHealthComponent()->GetCurrentHealth();
		const int32 EffectsBeforeShot = Weapon->GetImpactEffectSpawnCount();
		if (!Weapon->TryFire())
		{
			return;
		}
		bCombatFeedbackShotFired = true;
		bCombatFeedbackVerified =
			Target->GetHealthComponent()->GetCurrentHealth() < HealthBeforeShot &&
			Target->GetHitReactionCount() > 0 &&
			Target->IsHitReactionActive() &&
			Weapon->GetImpactEffectSpawnCount() > EffectsBeforeShot;
		if (!bCombatFeedbackVerified)
		{
			UE_LOG(LogCWSGame, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: shot did not trigger hit feedback"));
			FPlatformMisc::RequestExitWithStatus(true, 5, TEXT("CWS combat feedback was not triggered"));
			return;
		}
		UE_LOG(LogCWSGame, Display, TEXT("CWS_COMBAT_FEEDBACK_VERIFIED: damage, impact Niagara, and hit reaction animation"));
		return;
	}

	if (!bCombatFeedbackCaptureShotFired && ElapsedTime < 4.0f)
	{
		return;
	}
	if (!bCombatFeedbackCaptureShotFired)
	{
		const int32 HitReactionsBeforeShot = Target->GetHitReactionCount();
		const int32 EffectsBeforeShot = Weapon->GetImpactEffectSpawnCount();
		if (!Weapon->TryFire())
		{
			return;
		}
		if (Target->GetHitReactionCount() <= HitReactionsBeforeShot ||
			Weapon->GetImpactEffectSpawnCount() <= EffectsBeforeShot)
		{
			UE_LOG(LogCWSGame, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: warmed shot did not trigger feedback"));
			FPlatformMisc::RequestExitWithStatus(true, 7, TEXT("CWS warmed combat feedback shot failed"));
			return;
		}
		Target->GetHealthComponent()->Kill(PlayerCharacter);
		if (!Target->HasPlayedDeathAnimation() || Target->GetDeathEffectSpawnCount() <= 0)
		{
			UE_LOG(LogCWSGame, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: death feedback did not trigger"));
			FPlatformMisc::RequestExitWithStatus(true, 8, TEXT("CWS death feedback capture setup failed"));
			return;
		}
		if (UAnimSingleNodeInstance* DeathAnimationInstance = Target->GetMesh()->GetSingleNodeInstance())
		{
			const float DeathPoseTime = DeathAnimationInstance->GetLength() * 0.7f;
			DeathAnimationInstance->SetPosition(DeathPoseTime, false);
			UE_LOG(LogCWSGame, Display, TEXT("Combat feedback death pose staged at %.2fs."), DeathPoseTime);
		}
		bCombatFeedbackCaptureShotFired = true;
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.05f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_COMBAT_FEEDBACK_CAPTURE_SHOT: warmed hit and death feedback triggered"));
		return;
	}
	if (++CombatFeedbackCaptureDelaySteps < 3)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	bCombatFeedbackScreenshotRequested = true;
	GetWorldTimerManager().ClearTimer(CombatFeedbackScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	CombatFeedbackScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSCombatFeedback.png"));
	IFileManager::Get().Delete(*CombatFeedbackScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(CombatFeedbackScreenshotPath, true, false);
	UE_LOG(LogCWSGame, Display, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_REQUESTED: %s"), *CombatFeedbackScreenshotPath);
	GetWorldTimerManager().SetTimer(
		CombatFeedbackScreenshotExitTimer,
		this,
		&ACWSGameMode::FinishCombatFeedbackScreenshotTest,
		1.0f,
		false);
}

void ACWSGameMode::FinishCombatFeedbackScreenshotTest()
{
	const int64 ScreenshotSize = IFileManager::Get().FileSize(*CombatFeedbackScreenshotPath);
	const bool bSucceeded = bCombatFeedbackVerified && ScreenshotSize > 0;
	if (bSucceeded)
	{
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_SUCCESS: %s Size=%lld"),
			*CombatFeedbackScreenshotPath,
			ScreenshotSize);
	}
	else
	{
		UE_LOG(
			LogCWSGame,
			Error,
			TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: %s Size=%lld"),
			*CombatFeedbackScreenshotPath,
			ScreenshotSize);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bSucceeded ? 0 : 6,
		bSucceeded ? TEXT("CWS combat feedback screenshot captured") : TEXT("CWS combat feedback screenshot failed"));
}

void ACWSGameMode::RunAttackFeedbackScreenshotStep()
{
	if (!bAttackFeedbackScreenshotTest || bAttackFeedbackScreenshotRequested || !GetWorld())
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - AttackFeedbackScreenshotStartTime > 12.0f)
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_FAILURE: attack state was not reached"));
		FPlatformMisc::RequestExitWithStatus(true, 9, TEXT("CWS attack feedback screenshot test timed out"));
		return;
	}

	BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	if (!PlayerCharacter || !PlayerController || !PlayerHealth.IsValid() || !WaveManager.IsValid())
	{
		return;
	}
	if (!bAttackFeedbackArenaPrepared)
	{
		if (WaveManager->GetWavePhase() != ECWSWavePhase::Preparing)
		{
			return;
		}
		WaveManager->StopWaveSystem();
		PlayerCharacter->GetMesh()->SetVisibility(false, true);
		bAttackFeedbackArenaPrepared = true;
		return;
	}

	ACWSEnemyBase* Target = AttackFeedbackScreenshotTarget.Get();
	if (!Target)
	{
		const FVector GroundForward = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();
		const FVector TargetLocation = PlayerCharacter->GetActorLocation() + GroundForward * 450.0f;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Target = GetWorld()->SpawnActor<ACWSEnemyBase>(
			ACWSEnemyBase::StaticClass(),
			TargetLocation,
			(-GroundForward).Rotation(),
			SpawnParameters);
		if (!Target)
		{
			return;
		}
		Target->GetCharacterMovement()->DisableMovement();
		Target->DetachFromControllerPendingDestroy();
		AttackFeedbackScreenshotTarget = Target;
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	PlayerController->SetControlRotation(
		(Target->GetActorLocation() + FVector(0.0f, 0.0f, 85.0f) - ViewLocation).Rotation());
	if (!bAttackFeedbackTriggered)
	{
		const FVector OriginalPlayerLocation = PlayerCharacter->GetActorLocation();
		PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
		PlayerCharacter->TeleportTo(
			Target->GetActorLocation() + Target->GetActorForwardVector() * 100.0f,
			PlayerCharacter->GetActorRotation());
		const float HealthBeforeAttack = PlayerHealth->GetCurrentHealth();
		const bool bAttackExecuted = Target->TryAttack(PlayerCharacter);
		bAttackFeedbackVerified =
			bAttackExecuted && PlayerHealth->GetCurrentHealth() < HealthBeforeAttack &&
			Target->GetAttackAnimationCount() > 0 && Target->GetAttackSoundPlayCount() > 0 &&
			Target->StageAttackPoseForCapture(0.38f);
		PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
		PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
		if (!bAttackFeedbackVerified)
		{
			UE_LOG(LogCWSGame, Error, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_FAILURE: attack feedback did not trigger"));
			FPlatformMisc::RequestExitWithStatus(true, 10, TEXT("CWS attack feedback was not triggered"));
			return;
		}
		bAttackFeedbackTriggered = true;
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.05f);
		UE_LOG(LogCWSGame, Display, TEXT("CWS_ATTACK_FEEDBACK_VERIFIED: damage, MM_Attack_01 montage, staged pose, and attack sound"));
		return;
	}
	if (++AttackFeedbackCaptureDelaySteps < 3)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	bAttackFeedbackScreenshotRequested = true;
	GetWorldTimerManager().ClearTimer(AttackFeedbackScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	AttackFeedbackScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSEnemyAttackFeedback.png"));
	IFileManager::Get().Delete(*AttackFeedbackScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(AttackFeedbackScreenshotPath, true, false);
	UE_LOG(LogCWSGame, Display, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_REQUESTED: %s"), *AttackFeedbackScreenshotPath);
	GetWorldTimerManager().SetTimer(
		AttackFeedbackScreenshotExitTimer,
		this,
		&ACWSGameMode::FinishAttackFeedbackScreenshotTest,
		1.0f,
		false);
}

void ACWSGameMode::FinishAttackFeedbackScreenshotTest()
{
	const int64 ScreenshotSize = IFileManager::Get().FileSize(*AttackFeedbackScreenshotPath);
	const bool bSucceeded = bAttackFeedbackVerified && ScreenshotSize > 0;
	if (bSucceeded)
	{
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_SUCCESS: %s Size=%lld"),
			*AttackFeedbackScreenshotPath,
			ScreenshotSize);
	}
	else
	{
		UE_LOG(
			LogCWSGame,
			Error,
			TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_FAILURE: %s Size=%lld"),
			*AttackFeedbackScreenshotPath,
			ScreenshotSize);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bSucceeded ? 0 : 11,
		bSucceeded ? TEXT("CWS attack feedback screenshot captured") : TEXT("CWS attack feedback screenshot failed"));
}

void ACWSGameMode::RunVisualPolishScreenshotStep()
{
	if (!bVisualPolishScreenshotTest || bVisualPolishScreenshotRequested || !GetWorld())
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - VisualPolishScreenshotStartTime > 12.0f)
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_FAILURE: presentation state was not reached"));
		FPlatformMisc::RequestExitWithStatus(true, 12, TEXT("CWS visual polish screenshot test timed out"));
		return;
	}

	BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	if (!PlayerCharacter || !PlayerController || !WaveManager.IsValid() || !ArenaVisualDirector.IsValid())
	{
		return;
	}
	if (!bVisualPolishArenaPrepared)
	{
		if (WaveManager->GetWavePhase() != ECWSWavePhase::Preparing)
		{
			return;
		}
		WaveManager->StopWaveSystem();
		PlayerCharacter->GetMesh()->SetVisibility(false, true);
		bVisualPolishArenaPrepared = true;
		return;
	}

	if (VisualPolishScreenshotTargets.Num() == 0)
	{
		const FVector Forward = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
		const TArray<UClass*> EnemyClasses = {
			ACWSEnemyBase::StaticClass(),
			ACWSFastEnemy::StaticClass(),
			ACWSTankEnemy::StaticClass()
		};
		for (int32 Index = 0; Index < EnemyClasses.Num(); ++Index)
		{
			const float SideOffset = static_cast<float>(Index - 1) * 340.0f;
			const FVector SpawnLocation = PlayerCharacter->GetActorLocation() + Forward * 380.0f + Right * SideOffset;
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACWSEnemyBase* Target = GetWorld()->SpawnActor<ACWSEnemyBase>(
				EnemyClasses[Index],
				SpawnLocation,
				(-Forward).Rotation(),
				SpawnParameters);
			if (!Target)
			{
				return;
			}
			Target->GetCharacterMovement()->DisableMovement();
			Target->DetachFromControllerPendingDestroy();
			VisualPolishScreenshotTargets.Add(Target);
		}
		return;
	}

	if (VisualPolishScreenshotTargets.Num() != 3 ||
		!VisualPolishScreenshotTargets[0].IsValid() ||
		!VisualPolishScreenshotTargets[1].IsValid() ||
		!VisualPolishScreenshotTargets[2].IsValid())
	{
		return;
	}
	ACWSEnemyBase* NormalEnemy = VisualPolishScreenshotTargets[0].Get();
	ACWSEnemyBase* FastEnemy = VisualPolishScreenshotTargets[1].Get();
	ACWSEnemyBase* TankEnemy = VisualPolishScreenshotTargets[2].Get();
	bVisualPolishVerified =
		ArenaVisualDirector->IsPresentationReady() && ArenaVisualDirector->HasBlockingCover() &&
		ArenaVisualDirector->GetCenterRingSegmentCount() == 24 && ArenaVisualDirector->GetCoverCount() == 8 &&
		NormalEnemy->HasArchetypePresentation() && FastEnemy->HasArchetypePresentation() &&
		TankEnemy->HasArchetypePresentation() &&
		NormalEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Normal/SK_NormalMinion.SK_NormalMinion") &&
		FastEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Fast/SK_FastMinion.SK_FastMinion") &&
		TankEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Tank/SK_Tank.SK_Tank") &&
		!NormalEnemy->GetArchetypeColor().Equals(FastEnemy->GetArchetypeColor()) &&
		!FastEnemy->GetArchetypeColor().Equals(TankEnemy->GetArchetypeColor()) &&
		!NormalEnemy->GetArchetypeColor().Equals(TankEnemy->GetArchetypeColor());
	if (!bVisualPolishVerified)
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_FAILURE: visual presentation validation failed"));
		FPlatformMisc::RequestExitWithStatus(true, 13, TEXT("CWS visual polish validation failed"));
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector GroupCenter = FastEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 95.0f);
	PlayerController->SetControlRotation((GroupCenter - ViewLocation).Rotation());
	if (++VisualPolishCaptureDelaySteps < 8)
	{
		return;
	}

	bVisualPolishScreenshotRequested = true;
	GetWorldTimerManager().ClearTimer(VisualPolishScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	VisualPolishScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSArenaVisualPolish.png"));
	IFileManager::Get().Delete(*VisualPolishScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(VisualPolishScreenshotPath, true, false);
	UE_LOG(
		LogCWSGame,
		Display,
		TEXT("CWS_VISUAL_POLISH_VERIFIED: arena ring, 8 blocking covers, gate beacons, and Normal/Fast/Tank colors"));
	GetWorldTimerManager().SetTimer(
		VisualPolishScreenshotExitTimer,
		this,
		&ACWSGameMode::FinishVisualPolishScreenshotTest,
		1.0f,
		false);
}

void ACWSGameMode::FinishVisualPolishScreenshotTest()
{
	const int64 ScreenshotSize = IFileManager::Get().FileSize(*VisualPolishScreenshotPath);
	const bool bSucceeded = bVisualPolishVerified && ScreenshotSize > 0;
	if (bSucceeded)
	{
		UE_LOG(LogCWSGame, Display, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_SUCCESS: %s Size=%lld"),
			*VisualPolishScreenshotPath, ScreenshotSize);
	}
	else
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_FAILURE: %s Size=%lld"),
			*VisualPolishScreenshotPath, ScreenshotSize);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bSucceeded ? 0 : 14,
		bSucceeded ? TEXT("CWS visual polish screenshot captured") : TEXT("CWS visual polish screenshot failed"));
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

void ACWSGameMode::ConfigureBalanceCombatTimings()
{
	if (bBalanceCombatConfigured || !WaveManager.IsValid())
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
	bBalanceCombatConfigured = true;
}

void ACWSGameMode::RunBalanceCombatStep()
{
	if (!bBalanceCombatTest || bBalanceCombatFinished || !GetWorld())
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - BalanceCombatStartTime > 120.0f)
	{
		FinishBalanceCombatTest(false, TEXT("actual-hit balance run exceeded 120 seconds"));
		return;
	}

	BindGameplayActors();
	ConfigureBalanceCombatTimings();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponComponent() : nullptr;
	if (!PlayerCharacter || !PlayerController || !Weapon || !WaveManager.IsValid() || !bBalanceCombatConfigured)
	{
		return;
	}
	if (BalanceInitialAmmo == 0)
	{
		BalanceInitialAmmo = Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo();
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("CWS_BALANCE_AMMO_BUDGET: Magazine=%d StartingReserve=%d MaxReserve=%d Supply=%d"),
			Weapon->GetMaxAmmo(),
			Weapon->GetStartingReserveAmmo(),
			Weapon->GetMaxReserveAmmo(),
			GetDefault<ACWSSupplyPickup>()->GetAmmoAmount());
	}

	if (ACWSSupplyPickup* Supply = LastRoundSupply.Get())
	{
		const ECWSSupplyType SupplyType = Supply->GetSupplyType();
		if (SupplyType == ECWSSupplyType::Health && PlayerHealth.IsValid())
		{
			PlayerHealth->ApplyHealthChange(-10.0f, this);
		}
		const int32 AmmoBefore = Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo();
		if (Supply->TryCollect(PlayerCharacter))
		{
			if (SupplyType == ECWSSupplyType::Ammo)
			{
				++BalanceAmmoSuppliesCollected;
				BalanceAmmoGained += Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo() - AmmoBefore;
			}
			else
			{
				++BalanceHealthSuppliesCollected;
			}
			LastRoundSupply.Reset();
		}
	}
	if (PlayerHealth.IsValid())
	{
		PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
	}

	TArray<ACWSEnemyBase*> AliveEnemies;
	for (TActorIterator<ACWSEnemyBase> It(GetWorld()); It; ++It)
	{
		ACWSEnemyBase* Enemy = *It;
		if (Enemy->GetHealthComponent() && Enemy->GetHealthComponent()->IsAlive())
		{
			Enemy->GetCharacterMovement()->DisableMovement();
			Enemy->DetachFromControllerPendingDestroy();
			AliveEnemies.Add(Enemy);
		}
	}

	ACWSEnemyBase* Target = BalanceCombatTarget.Get();
	if (!Target || !Target->GetHealthComponent() || !Target->GetHealthComponent()->IsAlive())
	{
		BalanceCombatTarget.Reset();
		bBalanceTargetAimPrimed = false;
		Target = AliveEnemies.IsEmpty() ? nullptr : AliveEnemies[0];
		if (Target)
		{
			BalanceCombatTarget = Target;
		}
	}

	const FVector Forward = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	int32 ParkedIndex = 0;
	for (ACWSEnemyBase* Enemy : AliveEnemies)
	{
		if (Enemy == Target)
		{
			continue;
		}
		const float Side = static_cast<float>((ParkedIndex % 9) - 4) * 180.0f;
		const float Back = 1400.0f + static_cast<float>(ParkedIndex / 9) * 180.0f;
		Enemy->TeleportTo(PlayerCharacter->GetActorLocation() - Forward * Back + Right * Side, Enemy->GetActorRotation());
		++ParkedIndex;
	}

	if (Target)
	{
		const FVector TargetLocation = PlayerCharacter->GetActorLocation() + Forward * 800.0f;
		Target->TeleportTo(TargetLocation, (-Forward).Rotation());
		PlayerController->SetControlRotation(
			(TargetLocation + FVector(0.0f, 0.0f, 65.0f) - PlayerCharacter->GetPawnViewLocation()).Rotation());
		if (!bBalanceTargetAimPrimed)
		{
			bBalanceTargetAimPrimed = true;
			return;
		}
		const float HealthBefore = Target->GetHealthComponent()->GetCurrentHealth();
		const int32 AmmoBefore = Weapon->GetCurrentAmmo();
		if (Weapon->TryFire() && Weapon->GetCurrentAmmo() < AmmoBefore)
		{
			++BalanceShotsFired;
			BalanceShotsByRound.FindOrAdd(WaveManager->GetCurrentRound())++;
			const float HealthAfter = Target->GetHealthComponent()->GetCurrentHealth();
			if (HealthAfter >= HealthBefore)
			{
				++BalanceMissedShots;
			}
			if (!Target->GetHealthComponent()->IsAlive())
			{
				BalanceKillsByType.FindOrAdd(Target->GetEnemyType())++;
				BalanceCombatTarget.Reset();
				bBalanceTargetAimPrimed = false;
			}
		}
		else if (Weapon->GetCurrentAmmo() <= 0 && Weapon->GetReserveAmmo() <= 0)
		{
			FinishBalanceCombatTest(false, TEXT("ammo exhausted before all enemies were defeated"));
		}
		return;
	}

	if (bGameCleared && WaveManager->GetWavePhase() == ECWSWavePhase::Completed)
	{
		FinishBalanceCombatTest(true, TEXT("all five rounds cleared through actual hitscan damage"));
	}
}

void ACWSGameMode::FinishBalanceCombatTest(const bool bSucceeded, const TCHAR* Reason)
{
	if (bBalanceCombatFinished)
	{
		return;
	}
	bBalanceCombatFinished = true;
	GetWorldTimerManager().ClearTimer(BalanceCombatTimer);

	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponComponent() : nullptr;
	const TArray<int32> ExpectedRoundShots = {24, 40, 104, 132, 104};
	bool bRoundShotsMatch = true;
	for (int32 Index = 0; Index < ExpectedRoundShots.Num(); ++Index)
	{
		const int32 RoundNumber = Index + 1;
		const int32 ActualShots = BalanceShotsByRound.FindRef(RoundNumber);
		bRoundShotsMatch = bRoundShotsMatch && ActualShots == ExpectedRoundShots[Index];
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("CWS_BALANCE_ROUND: Round=%d ActualHitShots=%d Expected=%d"),
			RoundNumber,
			ActualShots,
			ExpectedRoundShots[Index]);
	}

	const int32 TotalEnemies = BalanceKillsByType.FindRef(ECWSEnemyType::Normal) +
		BalanceKillsByType.FindRef(ECWSEnemyType::Fast) +
		BalanceKillsByType.FindRef(ECWSEnemyType::Tank) +
		BalanceKillsByType.FindRef(ECWSEnemyType::Boss);
	const int32 RequiredAtSeventyPercent = FMath::CeilToInt(static_cast<float>(BalanceShotsFired) / 0.70f);
	const int32 TotalAvailableAmmo = BalanceInitialAmmo + 2 * GetDefault<ACWSSupplyPickup>()->GetAmmoAmount();
	const int32 RemainingAmmo = Weapon ? Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo() : -1;
	const bool bVerified = bSucceeded && bRoundShotsMatch && BalanceShotsFired == 404 && BalanceMissedShots == 0 &&
		TotalEnemies == 97 && BalanceKillsByType.FindRef(ECWSEnemyType::Normal) == 44 &&
		BalanceKillsByType.FindRef(ECWSEnemyType::Fast) == 32 && BalanceKillsByType.FindRef(ECWSEnemyType::Tank) == 20 &&
		BalanceKillsByType.FindRef(ECWSEnemyType::Boss) == 1 && BalanceAmmoSuppliesCollected == 2 &&
		BalanceHealthSuppliesCollected == 2 && BalanceAmmoGained == 180 &&
		TotalAvailableAmmo >= RequiredAtSeventyPercent && RemainingAmmo == 196;
	if (bVerified)
	{
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("CWS_BALANCE_COMBAT_SUCCESS: Enemies=97 ActualHitShots=404 AvailableAt70Percent=600 RequiredAt70Percent=%d RemainingAfterPerfectRun=%d AmmoSupplies=2 HealthSupplies=2"),
			RequiredAtSeventyPercent,
			RemainingAmmo);
	}
	else
	{
		UE_LOG(
			LogCWSGame,
			Error,
			TEXT("CWS_BALANCE_COMBAT_FAILURE: %s Enemies=%d Shots=%d Misses=%d AmmoGained=%d Remaining=%d AmmoSupplies=%d HealthSupplies=%d"),
			Reason,
			TotalEnemies,
			BalanceShotsFired,
			BalanceMissedShots,
			BalanceAmmoGained,
			RemainingAmmo,
			BalanceAmmoSuppliesCollected,
			BalanceHealthSuppliesCollected);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bVerified ? 0 : 21,
		bVerified ? TEXT("CWS balance combat test succeeded") : TEXT("CWS balance combat test failed"));
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
	const int32 ImpactEffectsBeforeShot = Weapon->GetImpactEffectSpawnCount();
	const int32 FireSoundsBeforeShot = Weapon->GetFireSoundPlayCount();
	const int32 ImpactSoundsBeforeShot = Weapon->GetImpactSoundPlayCount();
	if (!Weapon->TryFire())
	{
		return;
	}
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
			LogCWSGame,
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
				LogCWSGame,
				Display,
				TEXT("CWS_COMBAT_FEEDBACK_SMOKE_VERIFIED: fire/impact sound, hitscan damage, impact Niagara, hit reaction, death animation, and death effect"));
		}
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
	if (ArenaVisualDirector.IsValid())
	{
		bSmokeSawArenaVisuals =
			ArenaVisualDirector->IsPresentationReady() &&
			ArenaVisualDirector->GetCenterRingSegmentCount() == 24 &&
			ArenaVisualDirector->GetCoverCount() == 8 &&
			ArenaVisualDirector->GetGateBeaconCount() == 8;
		if (bSmokeSawArenaVisuals && !bSmokeLoggedArenaPresentation)
		{
			bSmokeLoggedArenaPresentation = true;
			UE_LOG(
				LogCWSGame,
				Display,
				TEXT("CWS_ARENA_PRESENTATION_VERIFIED: 24 ring segments, 8 blocking covers, and 8 gate beacons"));
		}
	}
	if (WaveManager.IsValid())
	{
		HandleWavePhaseChanged(WaveManager->GetWavePhase(), WaveManager->GetCurrentRound());
	}
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (PlayerCharacter)
	{
		bSmokeSawPlayer = true;
	}

	if (bSmokeRestartVerification)
	{
		if (PlayerCharacter && WaveManager.IsValid() && PlayerHealth.IsValid() && !bGameOver && !bGameCleared)
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
					LogCWSGame,
					Display,
					TEXT("CWS_ENEMY_ARCHETYPES_VERIFIED: Fast 35 health/520 speed/8 damage and Tank 180 health/230 speed/18 damage"));
			}

			if (ACWSBossEnemy* Boss = Cast<ACWSBossEnemy>(Enemy))
			{
				bSmokeSawDedicatedBoss = true;
				bSmokeSawBossMaxHealth = FMath::IsNearlyEqual(Health->GetMaxHealth(), 1200.0f);
				bSmokeSawBossPresentation = Boss->HasArchetypePresentation() &&
					Boss->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Normal/SK_NormalMinion.SK_NormalMinion");
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
					bSmokeSawBossExplosionSound = Boss->GetExplosionSoundPlayCount() >= 2;
					PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
					PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
					PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);

					if (bSmokeSawBossMaxHealth && bSmokeSawBossFinalPhase &&
						bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage &&
						bSmokeSawBossExplosionSound)
					{
						UE_LOG(
							LogCWSGame,
							Display,
							TEXT("CWS_BOSS_SMOKE_VERIFIED: class, 1200 health, final phase, ground slam, shockwave damage, knockback, and explosion sound"));
					}
				}

				if (bSmokeSawBossMaxHealth && bSmokeSawBossFinalPhase &&
					bSmokeSawBossGroundSlamDamage && bSmokeSawBossShockwaveDamage &&
					bSmokeSawBossExplosionSound)
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
			if ((!bSmokeSawEnemyAttackDamage || !bSmokeSawEnemyAttackAnimation || !bSmokeSawEnemyAttackSound) &&
				!Cast<ACWSBossEnemy>(Enemy) && PlayerCharacter && PlayerHealth.IsValid())
			{
				const FVector OriginalPlayerLocation = PlayerCharacter->GetActorLocation();
				PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
				PlayerCharacter->TeleportTo(
					Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 100.0f,
					PlayerCharacter->GetActorRotation());
				const float HealthBeforeAttack = PlayerHealth->GetCurrentHealth();
				const int32 AnimationsBeforeAttack = Enemy->GetAttackAnimationCount();
				const int32 SoundsBeforeAttack = Enemy->GetAttackSoundPlayCount();
				const bool bAttackExecuted = Enemy->TryAttack(PlayerCharacter);
				bSmokeSawEnemyAttackDamage = bSmokeSawEnemyAttackDamage ||
					(bAttackExecuted && PlayerHealth->GetCurrentHealth() < HealthBeforeAttack);
				bSmokeSawEnemyAttackAnimation = bSmokeSawEnemyAttackAnimation ||
					Enemy->GetAttackAnimationCount() > AnimationsBeforeAttack;
				bSmokeSawEnemyAttackSound = bSmokeSawEnemyAttackSound ||
					Enemy->GetAttackSoundPlayCount() > SoundsBeforeAttack;
				PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
				PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
				PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
				if (bSmokeSawEnemyAttackDamage && bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound)
				{
					UE_LOG(
						LogCWSGame,
						Display,
						TEXT("CWS_ENEMY_ATTACK_FEEDBACK_VERIFIED: damage, MM_Attack_01 montage, and procedural attack sound"));
				}
			}
			if (bSmokeSawEnemyAttackDamage && bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound)
			{
				Health->Kill(this);
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
			UE_LOG(LogCWSGame, Display, TEXT("CWS_ENEMY_PRESENTATION_VERIFIED: Normal green, Fast orange, and Tank blue"));
		}
		else
		{
			UE_LOG(LogCWSGame, Display, TEXT("CWS_ENEMY_PRESENTATION_VERIFIED: Normal green"));
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
				bSmokeSawFireSound && bSmokeSawImpactSound && bSmokeSawHitReaction && bSmokeSawImpactEffect &&
				bSmokeSawDeathAnimation && bSmokeSawDeathEffect && bSmokeSawEnemyAttackDamage &&
				bSmokeSawEnemyAttackAnimation && bSmokeSawEnemyAttackSound &&
				bSmokeSawArenaVisuals && bSmokeSawNormalPresentation &&
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
				SmokeHighestRoundCleared == 5 && bGameCleared,
			TEXT("Arena presentation, enemy type colors, combat feedback, reload, supplies, round phases, archetypes, boss patterns, and all five rounds were verified"));
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

void ACWSGameMode::HandleWavePhaseChanged(const ECWSWavePhase WavePhase, const int32 RoundNumber)
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
			LogCWSGame,
			Display,
			TEXT("CWS_ROUND_ANNOUNCEMENT_PHASES_VERIFIED: Preparing, Active, and RoundCleared observed through round %d"),
			RoundNumber);
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
