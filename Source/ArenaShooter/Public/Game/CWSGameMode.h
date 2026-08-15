#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Wave/CWSWaveTypes.h"
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
	void RunHudScreenshotStep();
	void FinishHudScreenshotTest();
	void SpawnRoundClearSupply(int32 RoundNumber);
	void FinishSmokeTest(bool bSucceeded, const TCHAR* Reason);

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
	TWeakObjectPtr<ACWSEnemyBase> SmokeWeaponTarget;
	TWeakObjectPtr<ACWSSupplyPickup> LastRoundSupply;
	TMap<TWeakObjectPtr<ACWSEnemyBase>, FVector> SmokeEnemyStartLocations;
	FTimerHandle GameplayBindTimer;
	FTimerHandle SmokeStepTimer;
	FTimerHandle HudScreenshotTimer;
	FTimerHandle HudScreenshotExitTimer;
	float SmokeStartTime = 0.0f;
	float HudScreenshotStartTime = 0.0f;
	FString HudScreenshotPath;
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
	bool bSmokeSawFastEnemy = false;
	bool bSmokeSawFastStats = false;
	bool bSmokeSawTankEnemy = false;
	bool bSmokeSawTankStats = false;
	bool bSmokeLoggedEnemyArchetypes = false;
	bool bSmokeSawPreparingPhase = false;
	bool bSmokeSawActivePhase = false;
	bool bSmokeSawRoundClearedPhase = false;
	bool bSmokeSawCompletedPhase = false;
	bool bSmokeLoggedRoundAnnouncementPhases = false;
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
	bool bHudScreenshotTest = false;
	bool bHudScreenshotRequested = false;
};
