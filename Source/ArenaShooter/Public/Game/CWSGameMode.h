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
class ACWSArenaVisualDirector;
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
	void RunCombatFeedbackScreenshotStep();
	void FinishCombatFeedbackScreenshotTest();
	void RunAttackFeedbackScreenshotStep();
	void FinishAttackFeedbackScreenshotTest();
	void RunVisualPolishScreenshotStep();
	void FinishVisualPolishScreenshotTest();
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
	TWeakObjectPtr<ACWSEnemyBase> CombatFeedbackScreenshotTarget;
	TWeakObjectPtr<ACWSEnemyBase> AttackFeedbackScreenshotTarget;
	TWeakObjectPtr<ACWSSupplyPickup> LastRoundSupply;
	TWeakObjectPtr<ACWSArenaVisualDirector> ArenaVisualDirector;
	TArray<TWeakObjectPtr<ACWSEnemyBase>> VisualPolishScreenshotTargets;
	TMap<TWeakObjectPtr<ACWSEnemyBase>, FVector> SmokeEnemyStartLocations;
	FTimerHandle GameplayBindTimer;
	FTimerHandle SmokeStepTimer;
	FTimerHandle HudScreenshotTimer;
	FTimerHandle HudScreenshotExitTimer;
	FTimerHandle CombatFeedbackScreenshotTimer;
	FTimerHandle CombatFeedbackScreenshotExitTimer;
	FTimerHandle AttackFeedbackScreenshotTimer;
	FTimerHandle AttackFeedbackScreenshotExitTimer;
	FTimerHandle VisualPolishScreenshotTimer;
	FTimerHandle VisualPolishScreenshotExitTimer;
	float SmokeStartTime = 0.0f;
	float HudScreenshotStartTime = 0.0f;
	float CombatFeedbackScreenshotStartTime = 0.0f;
	float AttackFeedbackScreenshotStartTime = 0.0f;
	float VisualPolishScreenshotStartTime = 0.0f;
	FString HudScreenshotPath;
	FString CombatFeedbackScreenshotPath;
	FString AttackFeedbackScreenshotPath;
	FString VisualPolishScreenshotPath;
	int32 SmokeHighestRoundCleared = 0;
	int32 CombatFeedbackCaptureDelaySteps = 0;
	int32 AttackFeedbackCaptureDelaySteps = 0;
	int32 VisualPolishCaptureDelaySteps = 0;
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
	bool bSmokeSawArenaVisuals = false;
	bool bSmokeSawNormalPresentation = false;
	bool bSmokeSawFastPresentation = false;
	bool bSmokeSawTankPresentation = false;
	bool bSmokeLoggedArenaPresentation = false;
	bool bSmokeLoggedEnemyPresentation = false;
	bool bSmokeLoggedEnemyArchetypes = false;
	bool bSmokeSawPreparingPhase = false;
	bool bSmokeSawActivePhase = false;
	bool bSmokeSawRoundClearedPhase = false;
	bool bSmokeSawCompletedPhase = false;
	bool bSmokeLoggedRoundAnnouncementPhases = false;
	bool bSmokeWeaponTargetSpawned = false;
	bool bSmokeWeaponAimPrimed = false;
	bool bSmokeSawWeaponDamage = false;
	bool bSmokeSawFireSound = false;
	bool bSmokeSawImpactSound = false;
	bool bSmokeWeaponTargetKilled = false;
	bool bSmokeSawHitReaction = false;
	bool bSmokeSawImpactEffect = false;
	bool bSmokeSawDeathAnimation = false;
	bool bSmokeSawDeathEffect = false;
	bool bSmokeSawEnemyAttackDamage = false;
	bool bSmokeSawEnemyAttackAnimation = false;
	bool bSmokeSawEnemyAttackSound = false;
	bool bSmokeSawBossExplosionSound = false;
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
	bool bCombatFeedbackScreenshotTest = false;
	bool bCombatFeedbackArenaPrepared = false;
	bool bCombatFeedbackAimPrimed = false;
	bool bCombatFeedbackShotFired = false;
	bool bCombatFeedbackCaptureShotFired = false;
	bool bCombatFeedbackScreenshotRequested = false;
	bool bCombatFeedbackVerified = false;
	bool bAttackFeedbackScreenshotTest = false;
	bool bAttackFeedbackArenaPrepared = false;
	bool bAttackFeedbackTriggered = false;
	bool bAttackFeedbackScreenshotRequested = false;
	bool bAttackFeedbackVerified = false;
	bool bVisualPolishScreenshotTest = false;
	bool bVisualPolishArenaPrepared = false;
	bool bVisualPolishScreenshotRequested = false;
	bool bVisualPolishVerified = false;
};
