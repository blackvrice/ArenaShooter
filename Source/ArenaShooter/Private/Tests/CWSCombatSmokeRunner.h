#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"
#include "Wave/CWSWaveTypes.h"

class ACWSEnemyBase;
class ACWSGameMode;
class ACWSPlayerCharacter;
class ACWSSupplyPickup;

class FCWSCombatSmokeRunner
{
public:
	explicit FCWSCombatSmokeRunner(ACWSGameMode& InOwner);
	~FCWSCombatSmokeRunner();
	bool StartFromCommandLine();

	void HandleRoundCleared(int32 RoundNumber);
	void HandleWavePhaseChanged(ECWSWavePhase WavePhase, int32 RoundNumber);
	void HandleAllRoundsCompleted();
	void HandlePlayerDeath();
	void HandleSupplySpawned(ACWSSupplyPickup* Supply);

private:
	void ConfigureAllRoundsSmokeTimings();
	void PrepareRoundClearHealthRestoreVerification();
	void PrepareSmokeWeaponTarget(ACWSPlayerCharacter* PlayerCharacter);
	void RunSmokeWeaponStep(ACWSPlayerCharacter* PlayerCharacter);
	void RunSmokeSupplyStep(ACWSPlayerCharacter* PlayerCharacter);
	void RunCombatSmokeStep();
	void FinishSmokeTest(bool bSucceeded, const TCHAR* Reason);

	ACWSGameMode& Owner;
	TWeakObjectPtr<ACWSEnemyBase> SmokeWeaponTarget;
	TWeakObjectPtr<ACWSSupplyPickup> LastRoundSupply;
	TMap<TWeakObjectPtr<ACWSEnemyBase>, FVector> SmokeEnemyStartLocations;
	FTimerHandle SmokeStepTimer;
	float SmokeStartTime = 0.0f;
	int32 SmokeHighestRoundCleared = 0;
	int32 SmokeAmmoBeforeReload = 0;
	int32 SmokeReserveBeforeReload = 0;
	int32 SmokeRoundHealthRestorePreparedForRound = 0;
	int32 SmokeRoundHealthRestorePreparedCount = 0;
	int32 SmokeRoundHealthRestoreVerifiedCount = 0;
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
	bool bSmokeSawBossPresentation = false;
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
};
