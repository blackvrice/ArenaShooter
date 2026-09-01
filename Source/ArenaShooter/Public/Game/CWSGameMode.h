#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Wave/CWSWaveTypes.h"
#include "CWSGameMode.generated.h"

class ACWSArenaVisualDirector;
class ACWSSupplyPickup;
class ACWSWaveManager;
class FCWSBalanceTestRunner;
class FCWSCombatSmokeRunner;
class FCWSGameplayTestCoordinator;
class FCWSScreenshotTestRunner;
class UCWSHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCWSGameFlowEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSSupplySpawnedEvent, AActor*, SupplyActor);

/**
 * Title -> Combat -> Clear/GameOver -> Restart의 최상위 게임 흐름만 조정합니다.
 *
 * 전투 규칙은 Player/Weapon/Enemy에, 라운드 상태는 WaveManager에 위임합니다.
 * GameMode는 월드에 배치된 객체를 연결하고 종료 조건과 라운드 보상만 중재하므로,
 * 자동 테스트도 실제 런타임 객체를 그대로 사용할 수 있습니다.
 */
UCLASS()
class ARENASHOOTER_API ACWSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACWSGameMode();
	virtual ~ACWSGameMode() override;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsGameStarted() const { return bGameStarted; }

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsWaitingForStart() const { return !bGameStarted && !bGameOver && !bGameCleared; }

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void StartGame();

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsGameCleared() const { return bGameCleared; }

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool CanRestart() const { return bGameOver || bGameCleared; }

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void RestartCurrentLevel();

	// HUD, Blueprint 연출, 테스트는 구현 세부사항 대신 게임 흐름 이벤트를 구독합니다.
	UPROPERTY(BlueprintAssignable, Category = "Game Flow|Events")
	FCWSGameFlowEvent OnGameOver;

	UPROPERTY(BlueprintAssignable, Category = "Game Flow|Events")
	FCWSGameFlowEvent OnGameCleared;

	UPROPERTY(BlueprintAssignable, Category = "Game Flow|Events")
	FCWSSupplySpawnedEvent OnSupplySpawned;

private:
	// Runner는 Production API를 넓히지 않고 검증에 필요한 내부 상태만 읽습니다.
	friend class FCWSBalanceTestRunner;
	friend class FCWSCombatSmokeRunner;
	friend class FCWSScreenshotTestRunner;

	/** World Partition/스폰 순서가 늦어도 연결되도록 짧은 타이머로 재시도합니다. */
	void BindGameplayActors();
	void TryStartWaveSystem();
	void SpawnRoundClearSupply(int32 RoundNumber);

	UFUNCTION()
	void HandleRoundCleared(int32 RoundNumber);

	UFUNCTION()
	void HandleWavePhaseChanged(ECWSWavePhase WavePhase, int32 RoundNumber);

	UFUNCTION()
	void HandleAllRoundsCompleted();

	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor);

	// 월드가 소유하는 Actor/Component이므로 GameMode가 수명을 연장하지 않습니다.
	TWeakObjectPtr<ACWSWaveManager> WaveManager;
	TWeakObjectPtr<UCWSHealthComponent> PlayerHealth;
	TWeakObjectPtr<ACWSArenaVisualDirector> ArenaVisualDirector;
	// UGameModeBase가 UObject가 아닌 테스트 조정자의 명시적 수명을 소유합니다.
	FCWSGameplayTestCoordinator* TestCoordinator = nullptr;
	FTimerHandle GameplayBindTimer;
	bool bSkipTitleScreen = false;
	bool bGameStarted = false;
	bool bWaveStartIssued = false;
	bool bGameOver = false;
	bool bGameCleared = false;
};
