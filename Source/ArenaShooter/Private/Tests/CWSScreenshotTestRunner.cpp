#include "Tests/CWSScreenshotTestRunner.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy/CWSEnemyBase.h"
#include "Enemy/CWSBossEnemy.h"
#include "Enemy/CWSFastEnemy.h"
#include "Enemy/CWSTankEnemy.h"
#include "EngineUtils.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/CWSPlayerCharacter.h"
#include "TimerManager.h"
#include "UI/CWSHUD.h"
#include "UnrealClient.h"
#include "Wave/CWSWaveManager.h"
#include "World/CWSArenaVisualDirector.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSScreenshotTests, Log, All);

namespace
{
	constexpr float ScreenshotWriteTimeoutSeconds = 10.0f;

	bool IsCompletePng(const FString& Path, int64& OutFileSize)
	{
		// 파일 존재만으로 성공 처리하지 않고 PNG 시작 signature와 마지막 IEND chunk를 확인한다.
		TArray<uint8> Bytes;
		OutFileSize = IFileManager::Get().FileSize(*Path);
		if (OutFileSize <= 20 || !FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() != OutFileSize)
		{
			return false;
		}

		static constexpr uint8 PngSignature[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
		static constexpr uint8 PngEndChunk[] = { 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };
		return FMemory::Memcmp(Bytes.GetData(), PngSignature, UE_ARRAY_COUNT(PngSignature)) == 0 &&
			FMemory::Memcmp(
				Bytes.GetData() + Bytes.Num() - UE_ARRAY_COUNT(PngEndChunk),
				PngEndChunk,
				UE_ARRAY_COUNT(PngEndChunk)) == 0;
	}
}

FCWSScreenshotTestRunner::FCWSScreenshotTestRunner(ACWSGameMode& InOwner)
    : Owner(InOwner)
{
}

FCWSScreenshotTestRunner::~FCWSScreenshotTestRunner() = default;

bool FCWSScreenshotTestRunner::StartFromCommandLine()
{
	// 각 플래그는 독립 시나리오이며, 활성 시나리오만 자신의 준비 타이머를 시작한다.
	bTitleScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSTitleScreenshotTest"));
    bHudScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSHUDScreenshotTest"));
    bCombatFeedbackScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSCombatFeedbackScreenshotTest"));
    bAttackFeedbackScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSAttackFeedbackScreenshotTest"));
    bVisualPolishScreenshotTest = FParse::Param(FCommandLine::Get(), TEXT("CWSVisualPolishScreenshotTest"));

    if (!bTitleScreenshotTest && !bHudScreenshotTest && !bCombatFeedbackScreenshotTest &&
        !bAttackFeedbackScreenshotTest && !bVisualPolishScreenshotTest)
    {
        return false;
    }

	if (bTitleScreenshotTest)
	{
		TitleScreenshotStartTime = Owner.GetWorld()->GetTimeSeconds();
		Owner.GetWorldTimerManager().SetTimer(
			TitleScreenshotTimer,
			FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunTitleScreenshotStep(); }),
			0.1f,
			true,
			0.1f);
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_TITLE_SCREENSHOT_STARTED"));
	}

    if (bHudScreenshotTest)
    {
        HudScreenshotStartTime = Owner.GetWorld()->GetTimeSeconds();
        Owner.GetWorldTimerManager().SetTimer(
            HudScreenshotTimer,
			FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunHudScreenshotStep(); }),
            0.1f,
            true,
            0.1f);
        UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_HUD_SCREENSHOT_STARTED"));
    }

    if (bCombatFeedbackScreenshotTest)
    {
        CombatFeedbackScreenshotStartTime = Owner.GetWorld()->GetTimeSeconds();
        Owner.GetWorldTimerManager().SetTimer(
            CombatFeedbackScreenshotTimer,
			FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunCombatFeedbackScreenshotStep(); }),
            0.05f,
            true,
            0.1f);
        UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_STARTED"));
    }

    if (bAttackFeedbackScreenshotTest)
    {
        AttackFeedbackScreenshotStartTime = Owner.GetWorld()->GetTimeSeconds();
        Owner.GetWorldTimerManager().SetTimer(
            AttackFeedbackScreenshotTimer,
			FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunAttackFeedbackScreenshotStep(); }),
            0.05f,
            true,
            0.1f);
        UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_STARTED"));
    }

    if (bVisualPolishScreenshotTest)
    {
        VisualPolishScreenshotStartTime = Owner.GetWorld()->GetTimeSeconds();
        Owner.GetWorldTimerManager().SetTimer(
            VisualPolishScreenshotTimer,
			FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunVisualPolishScreenshotStep(); }),
            0.05f,
            true,
            0.1f);
        UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_STARTED"));
    }

    return true;
}

void FCWSScreenshotTestRunner::RunTitleScreenshotStep()
{
	// Title 대기 상태와 HUD가 준비된 뒤 캡처를 요청하고 디스크 검증 단계로 넘긴다.
	if (!bTitleScreenshotTest || bTitleScreenshotRequested || !Owner.GetWorld())
	{
		return;
	}

	Owner.BindGameplayActors();
	const bool bTitleStateReady = Owner.IsWaitingForStart() && Owner.WaveManager.IsValid() &&
		Owner.WaveManager->GetCurrentRound() == 0 && !Owner.WaveManager->IsRoundInProgress() &&
		Owner.ArenaVisualDirector.IsValid() && Owner.ArenaVisualDirector->IsPresentationReady();
	if (!bTitleStateReady)
	{
		if (Owner.GetWorld()->GetTimeSeconds() - TitleScreenshotStartTime > 10.0f)
		{
			UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_TITLE_SCREENSHOT_FAILURE: title state was not reached"));
			FPlatformMisc::RequestExitWithStatus(true, 14, TEXT("CWS title screenshot test timed out"));
		}
		return;
	}

	bTitleScreenshotRequested = true;
	Owner.GetWorldTimerManager().ClearTimer(TitleScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	TitleScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSTitleScreen.png"));
	IFileManager::Get().Delete(*TitleScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(TitleScreenshotPath, true, false);
	TitleScreenshotRequestTime = Owner.GetWorld()->GetTimeSeconds();
	UE_LOG(
		LogCWSScreenshotTests,
		Display,
		TEXT("CWS_TITLE_SCREEN_VERIFIED: waiting for Enter, wave idle, arena presentation ready"));
	Owner.GetWorldTimerManager().SetTimer(
		TitleScreenshotExitTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { FinishTitleScreenshotTest(); }),
		0.25f,
		true,
		1.0f);
}

void FCWSScreenshotTestRunner::FinishTitleScreenshotTest()
{
	// 렌더 요청은 비동기이므로 timeout 안에서 완전한 PNG가 보일 때까지 기다린다.
	int64 ScreenshotSize = -1;
	const bool bPngComplete = IsCompletePng(TitleScreenshotPath, ScreenshotSize);
	if (!bPngComplete && Owner.GetWorld() &&
		Owner.GetWorld()->GetTimeSeconds() - TitleScreenshotRequestTime < ScreenshotWriteTimeoutSeconds)
	{
		return;
	}
	if (!bPngComplete)
	{
		Owner.GetWorldTimerManager().ClearTimer(TitleScreenshotExitTimer);
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_TITLE_SCREENSHOT_FAILURE: %s Size=%lld"), *TitleScreenshotPath, ScreenshotSize);
		FPlatformMisc::RequestExitWithStatus(true, 15, TEXT("CWS title screenshot was not written"));
		return;
	}

	if (!bTitleStartTriggered)
	{
		bTitleStartTriggered = true;
		Owner.StartGame();
		return;
	}

	const bool bStartFlowReady = Owner.IsGameStarted() && Owner.WaveManager.IsValid() &&
		Owner.WaveManager->GetCurrentRound() == 1 &&
		(Owner.WaveManager->GetWavePhase() == ECWSWavePhase::Preparing ||
			Owner.WaveManager->GetWavePhase() == ECWSWavePhase::Active);
	if (!bStartFlowReady && Owner.GetWorld() &&
		Owner.GetWorld()->GetTimeSeconds() - TitleScreenshotRequestTime < ScreenshotWriteTimeoutSeconds)
	{
		return;
	}

	Owner.GetWorldTimerManager().ClearTimer(TitleScreenshotExitTimer);
	if (bStartFlowReady)
	{
		UE_LOG(
			LogCWSScreenshotTests,
			Display,
			TEXT("CWS_TITLE_SCREENSHOT_SUCCESS: %s Size=%lld TitleToRoundOne=true"),
			*TitleScreenshotPath,
			ScreenshotSize);
		FPlatformMisc::RequestExitWithStatus(true, 0, TEXT("CWS title screen and start flow verified"));
	}
	else
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_TITLE_SCREENSHOT_FAILURE: title did not transition to Round 1"));
		FPlatformMisc::RequestExitWithStatus(true, 16, TEXT("CWS title start flow failed"));
	}
}

void FCWSScreenshotTestRunner::RunHudScreenshotStep()
{
	// Round Preparing 상태를 유지해 countdown과 기본 HUD가 함께 보이는 프레임을 만든다.
	if (!bHudScreenshotTest || bHudScreenshotRequested || !Owner.GetWorld())
	{
		return;
	}

	Owner.BindGameplayActors();
	if (Owner.ArenaVisualDirector.IsValid())
	{
		bHudSawArenaVisuals =
			Owner.ArenaVisualDirector->IsPresentationReady() &&
			Owner.ArenaVisualDirector->GetCenterRingSegmentCount() == 24 &&
			Owner.ArenaVisualDirector->GetCoverCount() == 8 &&
			Owner.ArenaVisualDirector->GetGateBeaconCount() == 8;
		if (bHudSawArenaVisuals && !bHudLoggedArenaPresentation)
		{
			bHudLoggedArenaPresentation = true;
			UE_LOG(
				LogCWSScreenshotTests,
				Display,
				TEXT("CWS_ARENA_PRESENTATION_VERIFIED: 24 ring segments, 8 blocking covers, and 8 gate beacons"));
		}
	}
	if (!Owner.WaveManager.IsValid() || Owner.WaveManager->GetWavePhase() != ECWSWavePhase::Preparing ||
		Owner.WaveManager->GetCurrentRound() != 1)
	{
		if (Owner.GetWorld()->GetTimeSeconds() - HudScreenshotStartTime > 10.0f)
		{
			UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_HUD_SCREENSHOT_FAILURE: Round 1 preparing phase was not reached"));
			FPlatformMisc::RequestExitWithStatus(true, 2, TEXT("CWS HUD screenshot test timed out"));
		}
		return;
	}

	bHudScreenshotRequested = true;
	Owner.GetWorldTimerManager().ClearTimer(HudScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	HudScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSRoundAnnouncement.png"));
	IFileManager::Get().Delete(*HudScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(HudScreenshotPath, true, false);
	HudScreenshotRequestTime = Owner.GetWorld()->GetTimeSeconds();
	UE_LOG(
		LogCWSScreenshotTests,
		Display,
		TEXT("CWS_HUD_SCREENSHOT_REQUESTED: %s Phase=%s Remaining=%.2f"),
		*HudScreenshotPath,
		*UEnum::GetValueAsString(Owner.WaveManager->GetWavePhase()),
		Owner.WaveManager->GetPhaseTimeRemaining());
	Owner.GetWorldTimerManager().SetTimer(
		HudScreenshotExitTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { FinishHudScreenshotTest(); }),
		0.25f,
		true,
		1.0f);
}

void FCWSScreenshotTestRunner::FinishHudScreenshotTest()
{
	// 캡처 파일이 완전히 닫힌 뒤에만 성공 marker와 정상 종료 코드를 기록한다.
	int64 ScreenshotSize = -1;
	const bool bPngComplete = IsCompletePng(HudScreenshotPath, ScreenshotSize);
	if (!bPngComplete && Owner.GetWorld() &&
		Owner.GetWorld()->GetTimeSeconds() - HudScreenshotRequestTime < ScreenshotWriteTimeoutSeconds)
	{
		return;
	}
	Owner.GetWorldTimerManager().ClearTimer(HudScreenshotExitTimer);
	const bool bSucceeded = bPngComplete;
	if (bSucceeded)
	{
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_HUD_SCREENSHOT_SUCCESS: %s Size=%lld"), *HudScreenshotPath, ScreenshotSize);
	}
	else
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_HUD_SCREENSHOT_FAILURE: %s Size=%lld"), *HudScreenshotPath, ScreenshotSize);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bSucceeded ? 0 : 3,
		bSucceeded ? TEXT("CWS HUD screenshot captured") : TEXT("CWS HUD screenshot was not written"));
}

void FCWSScreenshotTestRunner::RunCombatFeedbackScreenshotStep()
{
	// 고정 표적을 실제 hitscan으로 맞혀 피격 반응과 사망 버스트가 한 프레임에 남도록 준비한다.
	if (!bCombatFeedbackScreenshotTest || bCombatFeedbackScreenshotRequested || !Owner.GetWorld())
	{
		return;
	}

	const float ElapsedTime = Owner.GetWorld()->GetTimeSeconds() - CombatFeedbackScreenshotStartTime;
	if (ElapsedTime > 12.0f)
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: feedback state was not reached"));
		FPlatformMisc::RequestExitWithStatus(true, 4, TEXT("CWS combat feedback screenshot test timed out"));
		return;
	}

	Owner.BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(&Owner, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponComponent() : nullptr;
	if (!PlayerCharacter || !PlayerController || !Weapon || !Owner.WaveManager.IsValid())
	{
		return;
	}

	if (!bCombatFeedbackArenaPrepared)
	{
		if (Owner.WaveManager->GetWavePhase() != ECWSWavePhase::Preparing)
		{
			return;
		}
		Owner.WaveManager->StopWaveSystem();
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
		Target = Owner.GetWorld()->SpawnActor<ACWSEnemyBase>(
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
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("Combat feedback screenshot target spawned at %s."), *TargetLocation.ToCompactString());
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
			UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: shot did not trigger hit feedback"));
			FPlatformMisc::RequestExitWithStatus(true, 5, TEXT("CWS combat feedback was not triggered"));
			return;
		}
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_COMBAT_FEEDBACK_VERIFIED: damage, native impact burst, and hit reaction animation"));
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
			UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: warmed shot did not trigger feedback"));
			FPlatformMisc::RequestExitWithStatus(true, 7, TEXT("CWS warmed combat feedback shot failed"));
			return;
		}
		Target->GetHealthComponent()->Kill(PlayerCharacter);
		if (!Target->HasPlayedDeathAnimation() || Target->GetDeathEffectSpawnCount() <= 0)
		{
			UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_FAILURE: death feedback did not trigger"));
			FPlatformMisc::RequestExitWithStatus(true, 8, TEXT("CWS death feedback capture setup failed"));
			return;
		}
		if (UAnimSingleNodeInstance* DeathAnimationInstance = Target->GetMesh()->GetSingleNodeInstance())
		{
			const float DeathPoseTime = DeathAnimationInstance->GetLength() * 0.7f;
			DeathAnimationInstance->SetPosition(DeathPoseTime, false);
			UE_LOG(LogCWSScreenshotTests, Display, TEXT("Combat feedback death pose staged at %.2fs."), DeathPoseTime);
		}
		bCombatFeedbackCaptureShotFired = true;
		UGameplayStatics::SetGlobalTimeDilation(Owner.GetWorld(), 0.05f);
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_COMBAT_FEEDBACK_CAPTURE_SHOT: warmed hit and death feedback triggered"));
		return;
	}
	if (++CombatFeedbackCaptureDelaySteps < 3)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(Owner.GetWorld(), 1.0f);
	bCombatFeedbackScreenshotRequested = true;
	Owner.GetWorldTimerManager().ClearTimer(CombatFeedbackScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	CombatFeedbackScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSCombatFeedback.png"));
	IFileManager::Get().Delete(*CombatFeedbackScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(CombatFeedbackScreenshotPath, true, false);
	CombatFeedbackScreenshotRequestTime = Owner.GetWorld()->GetTimeSeconds();
	UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_REQUESTED: %s"), *CombatFeedbackScreenshotPath);
	Owner.GetWorldTimerManager().SetTimer(
		CombatFeedbackScreenshotExitTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { FinishCombatFeedbackScreenshotTest(); }),
		0.25f,
		true,
		1.0f);
}

void FCWSScreenshotTestRunner::FinishCombatFeedbackScreenshotTest()
{
	// Production 피드백 플래그와 완전한 PNG를 모두 만족해야 캡처 성공으로 본다.
	int64 ScreenshotSize = -1;
	const bool bPngComplete = IsCompletePng(CombatFeedbackScreenshotPath, ScreenshotSize);
	if (!bPngComplete && Owner.GetWorld() &&
		Owner.GetWorld()->GetTimeSeconds() - CombatFeedbackScreenshotRequestTime < ScreenshotWriteTimeoutSeconds)
	{
		return;
	}
	Owner.GetWorldTimerManager().ClearTimer(CombatFeedbackScreenshotExitTimer);
	const bool bSucceeded = bCombatFeedbackVerified && bPngComplete;
	if (bSucceeded)
	{
		UE_LOG(
			LogCWSScreenshotTests,
			Display,
			TEXT("CWS_COMBAT_FEEDBACK_SCREENSHOT_SUCCESS: %s Size=%lld"),
			*CombatFeedbackScreenshotPath,
			ScreenshotSize);
	}
	else
	{
		UE_LOG(
			LogCWSScreenshotTests,
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

void FCWSScreenshotTestRunner::RunAttackFeedbackScreenshotStep()
{
	// 일반 적의 실제 공격 경로를 실행한 뒤 읽기 쉬운 공격 pose 시점에 캡처한다.
	if (!bAttackFeedbackScreenshotTest || bAttackFeedbackScreenshotRequested || !Owner.GetWorld())
	{
		return;
	}
	if (Owner.GetWorld()->GetTimeSeconds() - AttackFeedbackScreenshotStartTime > 12.0f)
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_FAILURE: attack state was not reached"));
		FPlatformMisc::RequestExitWithStatus(true, 9, TEXT("CWS attack feedback screenshot test timed out"));
		return;
	}

	Owner.BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(&Owner, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	if (!PlayerCharacter || !PlayerController || !Owner.PlayerHealth.IsValid() || !Owner.WaveManager.IsValid())
	{
		return;
	}
	if (!bAttackFeedbackArenaPrepared)
	{
		if (Owner.WaveManager->GetWavePhase() != ECWSWavePhase::Preparing)
		{
			return;
		}
		Owner.WaveManager->StopWaveSystem();
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
		Target = Owner.GetWorld()->SpawnActor<ACWSEnemyBase>(
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
		Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
		PlayerCharacter->TeleportTo(
			Target->GetActorLocation() + Target->GetActorForwardVector() * 100.0f,
			PlayerCharacter->GetActorRotation());
		const float HealthBeforeAttack = Owner.PlayerHealth->GetCurrentHealth();
		const bool bAttackExecuted = Target->TryAttack(PlayerCharacter);
		bAttackFeedbackVerified =
			bAttackExecuted && Owner.PlayerHealth->GetCurrentHealth() < HealthBeforeAttack &&
			Target->GetAttackAnimationCount() > 0 && Target->GetAttackSoundPlayCount() > 0 &&
			Target->StageAttackPoseForCapture(0.38f);
		PlayerCharacter->TeleportTo(OriginalPlayerLocation, PlayerCharacter->GetActorRotation());
		Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
		if (!bAttackFeedbackVerified)
		{
			UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_FAILURE: attack feedback did not trigger"));
			FPlatformMisc::RequestExitWithStatus(true, 10, TEXT("CWS attack feedback was not triggered"));
			return;
		}
		bAttackFeedbackTriggered = true;
		UGameplayStatics::SetGlobalTimeDilation(Owner.GetWorld(), 0.05f);
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_ATTACK_FEEDBACK_VERIFIED: damage, MM_Attack_01 montage, staged pose, and attack sound"));
		return;
	}
	if (++AttackFeedbackCaptureDelaySteps < 3)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(Owner.GetWorld(), 1.0f);
	bAttackFeedbackScreenshotRequested = true;
	Owner.GetWorldTimerManager().ClearTimer(AttackFeedbackScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	AttackFeedbackScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSEnemyAttackFeedback.png"));
	IFileManager::Get().Delete(*AttackFeedbackScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(AttackFeedbackScreenshotPath, true, false);
	AttackFeedbackScreenshotRequestTime = Owner.GetWorld()->GetTimeSeconds();
	UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_REQUESTED: %s"), *AttackFeedbackScreenshotPath);
	Owner.GetWorldTimerManager().SetTimer(
		AttackFeedbackScreenshotExitTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { FinishAttackFeedbackScreenshotTest(); }),
		0.25f,
		true,
		1.0f);
}

void FCWSScreenshotTestRunner::FinishAttackFeedbackScreenshotTest()
{
	// 피해/애니메이션/사운드 관찰과 PNG 기록 완료를 함께 검증한다.
	int64 ScreenshotSize = -1;
	const bool bPngComplete = IsCompletePng(AttackFeedbackScreenshotPath, ScreenshotSize);
	if (!bPngComplete && Owner.GetWorld() &&
		Owner.GetWorld()->GetTimeSeconds() - AttackFeedbackScreenshotRequestTime < ScreenshotWriteTimeoutSeconds)
	{
		return;
	}
	Owner.GetWorldTimerManager().ClearTimer(AttackFeedbackScreenshotExitTimer);
	const bool bSucceeded = bAttackFeedbackVerified && bPngComplete;
	if (bSucceeded)
	{
		UE_LOG(
			LogCWSScreenshotTests,
			Display,
			TEXT("CWS_ATTACK_FEEDBACK_SCREENSHOT_SUCCESS: %s Size=%lld"),
			*AttackFeedbackScreenshotPath,
			ScreenshotSize);
	}
	else
	{
		UE_LOG(
			LogCWSScreenshotTests,
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

void FCWSScreenshotTestRunner::RunVisualPolishScreenshotStep()
{
	// Arena 중앙에 모든 아키타입을 배치하고 Boss를 Final Phase로 만든 대표 구도를 준비한다.
	if (!bVisualPolishScreenshotTest || bVisualPolishScreenshotRequested || !Owner.GetWorld())
	{
		return;
	}
	if (Owner.GetWorld()->GetTimeSeconds() - VisualPolishScreenshotStartTime > 12.0f)
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_FAILURE: presentation state was not reached"));
		FPlatformMisc::RequestExitWithStatus(true, 12, TEXT("CWS visual polish screenshot test timed out"));
		return;
	}

	Owner.BindGameplayActors();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(&Owner, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	if (!PlayerCharacter || !PlayerController || !Owner.WaveManager.IsValid() || !Owner.ArenaVisualDirector.IsValid())
	{
		return;
	}
	if (!bVisualPolishArenaPrepared)
	{
		if (Owner.WaveManager->GetWavePhase() != ECWSWavePhase::Preparing)
		{
			return;
		}
		Owner.WaveManager->StopWaveSystem();
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
			ACWSTankEnemy::StaticClass(),
			ACWSBossEnemy::StaticClass()
		};
		for (int32 Index = 0; Index < EnemyClasses.Num(); ++Index)
		{
			const float SideOffset = (static_cast<float>(Index) - 1.5f) * 280.0f;
			const FVector SpawnLocation = PlayerCharacter->GetActorLocation() + Forward * 520.0f + Right * SideOffset;
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACWSEnemyBase* Target = Owner.GetWorld()->SpawnActor<ACWSEnemyBase>(
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
			if (ACWSBossEnemy* Boss = Cast<ACWSBossEnemy>(Target))
			{
				const float DamageToFinalPhase = Boss->GetHealthComponent()->GetMaxHealth() * 0.75f;
				Boss->GetHealthComponent()->ApplyHealthChange(-DamageToFinalPhase, &Owner);
			}
			VisualPolishScreenshotTargets.Add(Target);
		}
		return;
	}

	if (VisualPolishScreenshotTargets.Num() != 4 ||
		!VisualPolishScreenshotTargets[0].IsValid() ||
		!VisualPolishScreenshotTargets[1].IsValid() ||
		!VisualPolishScreenshotTargets[2].IsValid() ||
		!VisualPolishScreenshotTargets[3].IsValid())
	{
		return;
	}
	ACWSEnemyBase* NormalEnemy = VisualPolishScreenshotTargets[0].Get();
	ACWSEnemyBase* FastEnemy = VisualPolishScreenshotTargets[1].Get();
	ACWSEnemyBase* TankEnemy = VisualPolishScreenshotTargets[2].Get();
	ACWSBossEnemy* BossEnemy = Cast<ACWSBossEnemy>(VisualPolishScreenshotTargets[3].Get());
	bVisualPolishVerified =
		Owner.ArenaVisualDirector->IsPresentationReady() && Owner.ArenaVisualDirector->HasBlockingCover() &&
		Owner.ArenaVisualDirector->GetCenterRingSegmentCount() == 24 && Owner.ArenaVisualDirector->GetCoverCount() == 8 &&
		NormalEnemy->HasArchetypePresentation() && FastEnemy->HasArchetypePresentation() &&
		TankEnemy->HasArchetypePresentation() && BossEnemy && BossEnemy->HasBossPresentation() &&
		NormalEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Normal/SK_NormalMinion.SK_NormalMinion") &&
		FastEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Fast/SK_FastMinion.SK_FastMinion") &&
		TankEnemy->GetVisualMeshPath() == TEXT("/Game/CWSResources/Enemies/Tank/SK_Tank.SK_Tank") &&
		!NormalEnemy->GetArchetypeColor().Equals(FastEnemy->GetArchetypeColor()) &&
		!FastEnemy->GetArchetypeColor().Equals(TankEnemy->GetArchetypeColor()) &&
		!NormalEnemy->GetArchetypeColor().Equals(TankEnemy->GetArchetypeColor()) &&
		BossEnemy->GetBossPhase() == ECWSBossPhase::FinalPhase;
	if (!bVisualPolishVerified)
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_FAILURE: visual presentation validation failed"));
		FPlatformMisc::RequestExitWithStatus(true, 13, TEXT("CWS visual polish validation failed"));
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector GroupCenter =
		(FastEnemy->GetActorLocation() + TankEnemy->GetActorLocation()) * 0.5f + FVector(0.0f, 0.0f, 115.0f);
	PlayerController->SetControlRotation((GroupCenter - ViewLocation).Rotation());
	if (++VisualPolishCaptureDelaySteps < 8)
	{
		return;
	}

	bVisualPolishScreenshotRequested = true;
	Owner.GetWorldTimerManager().ClearTimer(VisualPolishScreenshotTimer);
	const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	VisualPolishScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("CWSArenaVisualPolish.png"));
	IFileManager::Get().Delete(*VisualPolishScreenshotPath, false, true);
	FScreenshotRequest::RequestScreenshot(VisualPolishScreenshotPath, true, false);
	VisualPolishScreenshotRequestTime = Owner.GetWorld()->GetTimeSeconds();
	UE_LOG(
		LogCWSScreenshotTests,
		Display,
		TEXT("CWS_VISUAL_POLISH_VERIFIED: arena ring, 8 blocking covers, gate beacons, Normal/Fast/Tank colors, and final-phase Boss aura"));
	Owner.GetWorldTimerManager().SetTimer(
		VisualPolishScreenshotExitTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { FinishVisualPolishScreenshotTest(); }),
		0.25f,
		true,
		1.0f);
}

void FCWSScreenshotTestRunner::FinishVisualPolishScreenshotTest()
{
	// 배치/색상/Boss 표현 계약과 최종 PNG 무결성을 함께 확인한다.
	int64 ScreenshotSize = -1;
	const bool bPngComplete = IsCompletePng(VisualPolishScreenshotPath, ScreenshotSize);
	if (!bPngComplete && Owner.GetWorld() &&
		Owner.GetWorld()->GetTimeSeconds() - VisualPolishScreenshotRequestTime < ScreenshotWriteTimeoutSeconds)
	{
		return;
	}
	Owner.GetWorldTimerManager().ClearTimer(VisualPolishScreenshotExitTimer);
	const bool bSucceeded = bVisualPolishVerified && bPngComplete;
	if (bSucceeded)
	{
		UE_LOG(LogCWSScreenshotTests, Display, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_SUCCESS: %s Size=%lld"),
			*VisualPolishScreenshotPath, ScreenshotSize);
	}
	else
	{
		UE_LOG(LogCWSScreenshotTests, Error, TEXT("CWS_VISUAL_POLISH_SCREENSHOT_FAILURE: %s Size=%lld"),
			*VisualPolishScreenshotPath, ScreenshotSize);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bSucceeded ? 0 : 14,
		bSucceeded ? TEXT("CWS visual polish screenshot captured") : TEXT("CWS visual polish screenshot failed"));
}
