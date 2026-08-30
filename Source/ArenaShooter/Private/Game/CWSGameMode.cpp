#include "Game/CWSGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/CWSSupplyPickup.h"
#include "Player/CWSPlayerCharacter.h"
#include "Tests/CWSGameplayTestCoordinator.h"
#include "TimerManager.h"
#include "UI/CWSHUD.h"
#include "Wave/CWSWaveManager.h"
#include "World/CWSArenaVisualDirector.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSGame, Log, All);

ACWSGameMode::ACWSGameMode()
{
	DefaultPawnClass = ACWSPlayerCharacter::StaticClass();
	HUDClass = ACWSHUD::StaticClass();
}

ACWSGameMode::~ACWSGameMode()
{
	delete TestCoordinator;
}

void ACWSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bSkipTitleScreen = UGameplayStatics::HasOption(Options, TEXT("AutoStart"));
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

	TestCoordinator = new FCWSGameplayTestCoordinator(*this);
	const bool bAutomatedTestStarted = TestCoordinator->StartFromCommandLine();
	const bool bKeepTitleScreen = TestCoordinator->ShouldKeepTitleScreen();
	if (bSkipTitleScreen || (bAutomatedTestStarted && !bKeepTitleScreen))
	{
		StartGame();
	}
	else
	{
		UE_LOG(LogCWSGame, Display, TEXT("CWS_TITLE_SCREEN_READY: Press Enter to start."));
	}
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
			if (ACWSPlayerCharacter* PlayerCharacter =
				Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
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

	if (bGameStarted && WaveManager.IsValid())
	{
		TryStartWaveSystem();
	}

	if (WaveManager.IsValid() && PlayerHealth.IsValid())
	{
		GetWorldTimerManager().ClearTimer(GameplayBindTimer);
	}
}

void ACWSGameMode::StartGame()
{
	if (bGameStarted || bGameOver || bGameCleared)
	{
		return;
	}

	bGameStarted = true;
	TryStartWaveSystem();
	UE_LOG(LogCWSGame, Display, TEXT("CWS_GAME_STARTED: Title screen confirmed and wave gameplay started."));
}

void ACWSGameMode::TryStartWaveSystem()
{
	if (!bGameStarted || bWaveStartIssued || !WaveManager.IsValid())
	{
		return;
	}

	WaveManager->StartWaveSystem();
	bWaveStartIssued = WaveManager->IsWaveSystemStarted();
}

void ACWSGameMode::HandleRoundCleared(const int32 RoundNumber)
{
	SpawnRoundClearSupply(RoundNumber);
	if (TestCoordinator)
	{
		TestCoordinator->HandleRoundCleared(RoundNumber);
	}
}

void ACWSGameMode::HandleWavePhaseChanged(const ECWSWavePhase WavePhase, const int32 RoundNumber)
{
	if (TestCoordinator)
	{
		TestCoordinator->HandleWavePhaseChanged(WavePhase, RoundNumber);
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
	OnSupplySpawned.Broadcast(Supply);
	if (TestCoordinator)
	{
		TestCoordinator->HandleSupplySpawned(Supply);
	}
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
	if (TestCoordinator)
	{
		TestCoordinator->HandleAllRoundsCompleted();
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
	if (TestCoordinator)
	{
		TestCoordinator->HandlePlayerDeath();
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
		UGameplayStatics::OpenLevel(this, FName(*LevelName), false, TEXT("AutoStart=1"));
	}
}
