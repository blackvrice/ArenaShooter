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

UCLASS()
class ARENASHOOTER_API ACWSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACWSGameMode();
	virtual ~ACWSGameMode() override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsGameCleared() const { return bGameCleared; }

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool CanRestart() const { return bGameOver || bGameCleared; }

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void RestartCurrentLevel();

	UPROPERTY(BlueprintAssignable, Category = "Game Flow|Events")
	FCWSGameFlowEvent OnGameOver;

	UPROPERTY(BlueprintAssignable, Category = "Game Flow|Events")
	FCWSGameFlowEvent OnGameCleared;

	UPROPERTY(BlueprintAssignable, Category = "Game Flow|Events")
	FCWSSupplySpawnedEvent OnSupplySpawned;

private:
	friend class FCWSBalanceTestRunner;
	friend class FCWSCombatSmokeRunner;
	friend class FCWSScreenshotTestRunner;

	void BindGameplayActors();
	void SpawnRoundClearSupply(int32 RoundNumber);

	UFUNCTION()
	void HandleRoundCleared(int32 RoundNumber);

	UFUNCTION()
	void HandleWavePhaseChanged(ECWSWavePhase WavePhase, int32 RoundNumber);

	UFUNCTION()
	void HandleAllRoundsCompleted();

	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor);

	TWeakObjectPtr<ACWSWaveManager> WaveManager;
	TWeakObjectPtr<UCWSHealthComponent> PlayerHealth;
	TWeakObjectPtr<ACWSArenaVisualDirector> ArenaVisualDirector;
	FCWSGameplayTestCoordinator* TestCoordinator = nullptr;
	FTimerHandle GameplayBindTimer;
	bool bGameOver = false;
	bool bGameCleared = false;
};
