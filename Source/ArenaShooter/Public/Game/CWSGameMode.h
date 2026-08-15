#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CWSGameMode.generated.h"

class ACWSEnemyBase;
class ACWSBossEnemy;
class ACWSWaveManager;
class ACWSPlayerCharacter;
class ACWSSupplyPickup;
class UCWSHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCWSGameFlowEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSSupplySpawnedEvent, AActor*, SupplyActor);

UCLASS()
class ARENASHOOTER_API ACWSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACWSGameMode();

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
	void BindGameplayActors();
	void ConfigureAllRoundsSmokeTimings();
	void PrepareSmokeWeaponTarget(ACWSPlayerCharacter* PlayerCharacter);
	void RunSmokeWeaponStep(ACWSPlayerCharacter* PlayerCharacter);
	void RunSmokeSupplyStep(ACWSPlayerCharacter* PlayerCharacter);
	void RunCombatSmokeStep();
	void SpawnRoundClearSupply(int32 RoundNumber);
	void FinishSmokeTest(bool bSucceeded, const TCHAR* Reason);

	UFUNCTION()
	void HandleRoundCleared(int32 RoundNumber);

	UFUNCTION()
	void HandleAllRoundsCompleted();

	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor);

	TWeakObjectPtr<ACWSWaveManager> WaveManager;
	TWeakObjectPtr<UCWSHealthComponent> PlayerHealth;
	TWeakObjectPtr<ACWSEnemyBase> SmokeWeaponTarget;
	TWeakObjectPtr<ACWSSupplyPickup> LastRoundSupply;
	TMap<TWeakObjectPtr<ACWSEnemyBase>, FVector> SmokeEnemyStartLocations;
	FTimerHandle GameplayBindTimer;
	FTimerHandle SmokeStepTimer;
	float SmokeStartTime = 0.0f;
	int32 SmokeHighestRoundCleared = 0;
	int32 SmokeAmmoBeforeReload = 0;
	int32 SmokeReserveBeforeReload = 0;
	bool bGameOver = false;
	bool bGameCleared = false;
	bool bSmokeTestEnabled = false;
	bool bSmokeTestAllRounds = false;
	bool bSmokeRestartVerification = false;
	bool bSmokeTimingsConfigured = false;
	bool bSmokeSawPlayer = false;
	bool bSmokeSawEnemyMovement = false;
	bool bSmokeSawDedicatedBoss = false;
	bool bSmokeSawBossMaxHealth = false;
	bool bSmokeSawBossFinalPhase = false;
	bool bSmokeSawBossGroundSlamDamage = false;
	bool bSmokeSawBossShockwaveDamage = false;
	bool bSmokeWeaponTargetSpawned = false;
	bool bSmokeWeaponAimPrimed = false;
	bool bSmokeSawWeaponDamage = false;
	bool bSmokeWeaponTargetKilled = false;
	bool bSmokeReloadStarted = false;
	bool bSmokeReloadCompleted = false;
	bool bSmokeAmmoSupplyCollected = false;
	bool bSmokeHealthSupplyCollected = false;
	bool bSmokeRoundOneCleared = false;
	bool bSmokeStoppedAfterRoundOne = false;
	bool bSmokeAllRoundsCleared = false;
	bool bSmokeAppliedPlayerDamage = false;
	bool bSmokeSawPlayerDeath = false;
	bool bSmokeFinished = false;
};
