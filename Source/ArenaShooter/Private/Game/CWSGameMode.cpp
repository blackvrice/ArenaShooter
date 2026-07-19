#include "Game/CWSGameMode.h"

#include "Components/CWSHealthComponent.h"
#include "Enemy/CWSEnemyBase.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CWSPlayerCharacter.h"
#include "TimerManager.h"
#include "UI/CWSHUD.h"
#include "Wave/CWSWaveManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSGame, Log, All);

ACWSGameMode::ACWSGameMode()
{
	DefaultPawnClass = ACWSPlayerCharacter::StaticClass();
	HUDClass = ACWSHUD::StaticClass();
}

void ACWSGameMode::BeginPlay()
{
	Super::BeginPlay();

	bSmokeTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("CWSRoundOneSmokeTest"));
	if (!bSmokeTestEnabled)
	{
		return;
	}

	SmokeStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(
		SmokeStepTimer,
		this,
		&ACWSGameMode::RunRoundOneSmokeStep,
		0.1f,
		true,
		0.1f);
	UE_LOG(LogCWSGame, Display, TEXT("CWS_ROUND_ONE_SMOKE_STARTED"));
}

void ACWSGameMode::RunRoundOneSmokeStep()
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

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		bSmokeSawPlayer = PlayerPawn->IsA<ACWSPlayerCharacter>();
	}

	if (!SmokeWaveManager.IsValid())
	{
		for (TActorIterator<ACWSWaveManager> It(World); It; ++It)
		{
			SmokeWaveManager = *It;
			It->OnRoundCleared.AddDynamic(this, &ACWSGameMode::HandleRoundCleared);
			break;
		}
	}

	for (TActorIterator<ACWSEnemyBase> It(World); It; ++It)
	{
		ACWSEnemyBase* Enemy = *It;
		UCWSHealthComponent* Health = Enemy->GetHealthComponent();
		if (!Health || !Health->IsAlive())
		{
			continue;
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

	if (World->GetTimeSeconds() - SmokeStartTime > 35.0f)
	{
		FinishSmokeTest(false, TEXT("Round 1 did not clear within 35 seconds"));
	}
}

void ACWSGameMode::HandleRoundCleared(const int32 RoundNumber)
{
	if (!bSmokeTestEnabled || RoundNumber != 1)
	{
		return;
	}

	if (!bSmokeSawPlayer)
	{
		FinishSmokeTest(false, TEXT("Native player pawn was not spawned"));
		return;
	}
	if (!bSmokeSawEnemyMovement)
	{
		FinishSmokeTest(false, TEXT("No enemy NavMesh movement was observed"));
		return;
	}
	FinishSmokeTest(true, TEXT("Round 1 spawned, navigated, died, and cleared"));
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
		UE_LOG(LogCWSGame, Display, TEXT("CWS_ROUND_ONE_SMOKE_SUCCESS: %s"), Reason);
		FPlatformMisc::RequestExitWithStatus(false, 0, TEXT("CWS round one smoke test succeeded"));
	}
	else
	{
		UE_LOG(LogCWSGame, Error, TEXT("CWS_ROUND_ONE_SMOKE_FAILURE: %s"), Reason);
		FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("CWS round one smoke test failed"));
	}
}
