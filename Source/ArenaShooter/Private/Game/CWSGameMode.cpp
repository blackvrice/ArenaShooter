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

	// World Partition 로드와 Pawn 생성 순서가 일정하지 않으므로 즉시 한 번 찾고,
	// 필수 객체가 모두 연결될 때까지 짧은 간격으로 재시도한다.
	BindGameplayActors();
	GetWorldTimerManager().SetTimer(
		GameplayBindTimer,
		this,
		&ACWSGameMode::BindGameplayActors,
		0.1f,
		true,
		0.1f);

	// 자동 검증도 별도 가짜 월드가 아니라 아래에서 연결한 실제 게임 객체를 사용한다.
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
	// WaveManager와 PlayerHealth는 각자 자신의 상태를 소유한다. GameMode는 완료/사망
	// 이벤트만 구독해 상위 게임 흐름으로 번역한다.
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

	// 맵에 Director가 없으면 플레이어 캡슐 바닥 높이에 런타임 표현 계층을 만든다.
	// 저장된 World Partition 액터를 수정하지 않아 Editor와 Shipping 구성이 동일하다.
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
	// 다음 라운드가 즉시 어려워지더라도 이전 라운드 피해가 누적되지 않도록 전투가
	// 끝난 경계에서만 회복한다. 사망 상태는 ApplyHealthChange로 부활시키지 않는다.
	if (!bGameOver && PlayerHealth.IsValid() && PlayerHealth->IsAlive())
	{
		const float RestoredHealth = PlayerHealth->ApplyHealthChange(PlayerHealth->GetMaxHealth(), this);
		UE_LOG(
			LogCWSGame,
			Display,
			TEXT("CWS_ROUND_CLEAR_HEALTH_RESTORED: Round=%d Health=%.1f/%.1f Restored=%.1f"),
			RoundNumber,
			PlayerHealth->GetCurrentHealth(),
			PlayerHealth->GetMaxHealth(),
			RestoredHealth);
	}

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
	// 마지막 라운드는 곧바로 Clear로 끝나므로 보급이 필요하지 않다.
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

	// 홀수 라운드는 다음 전투의 탄약을, 짝수 라운드는 생존 여유를 제공한다.
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

	// 먼저 웨이브를 멈춰 사망 화면 뒤에서 적/타이머가 계속 진행되지 않게 한다.
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
		// Restart에서는 Title을 다시 기다리지 않고 곧바로 전투 검증 가능한 상태로 연다.
		UGameplayStatics::OpenLevel(this, FName(*LevelName), false, TEXT("AutoStart=1"));
	}
}
