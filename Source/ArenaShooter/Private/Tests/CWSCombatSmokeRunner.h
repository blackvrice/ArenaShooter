#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"
#include "Wave/CWSWaveTypes.h"

class ACWSEnemyBase;
class ACWSGameMode;
class ACWSPlayerCharacter;
class ACWSSupplyPickup;

/**
 * 실제 게임 월드에서 전투 루프의 관찰 가능한 계약을 순서대로 검사합니다.
 *
 * 기본 모드는 Round 1의 이동/사격/재장전/보급/GameOver/Restart를 확인하고,
 * -AllRounds는 적 아키타입, 매 라운드 클리어, Boss 패턴과 Completed까지 확장합니다.
 */
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
	// 검증 단계: 시간 설정 -> 실제 표적 준비/사격 -> 보급 -> 전체 상태 종합 판정.
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
	// 라운드/탄약/체력 전후 값을 저장해 이벤트 발생뿐 아니라 결과값도 확인합니다.
	float SmokeStartTime = 0.0f;
	int32 SmokeHighestRoundCleared = 0;
	int32 SmokeAmmoBeforeReload = 0;
	int32 SmokeReserveBeforeReload = 0;
	int32 SmokeRoundHealthRestorePreparedForRound = 0;
	int32 SmokeRoundHealthRestorePreparedCount = 0;
	int32 SmokeRoundHealthRestoreVerifiedCount = 0;
	// 실행 모드와 Runner 수명주기 플래그입니다.
	bool bSmokeTestEnabled = false;
	bool bSmokeTestAllRounds = false;
	bool bSmokeRestartVerification = false;
	bool bSmokeTimingsConfigured = false;
	// 월드/적 타입/Boss/프레젠테이션 계약 관찰값입니다.
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
	// Wave 상태 머신이 올바른 순서로 노출됐는지 기록합니다.
	bool bSmokeSawPreparingPhase = false;
	bool bSmokeSawActivePhase = false;
	bool bSmokeSawRoundClearedPhase = false;
	bool bSmokeSawCompletedPhase = false;
	bool bSmokeLoggedRoundAnnouncementPhases = false;
	// 실제 Weapon과 전투 피드백 경로의 단계별 관찰값입니다.
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
	// 재장전/보급/게임 종료와 Restart 계약 관찰값입니다.
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
