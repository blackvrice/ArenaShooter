#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"

class ACWSEnemyBase;
class ACWSGameMode;
class ACWSSupplyPickup;

/**
 * 실제 Weapon::TryFire 경로로 전 라운드를 진행해 탄약 경제를 검증합니다.
 * 즉사나 직접 체력 조작 대신 조준, 발사 간격, 재장전, 보급을 Production 규칙대로 사용합니다.
 */
class FCWSBalanceTestRunner
{
public:
	explicit FCWSBalanceTestRunner(ACWSGameMode& InOwner);
	~FCWSBalanceTestRunner();
	bool StartFromCommandLine();
	void HandleSupplySpawned(ACWSSupplyPickup* Supply);

private:
	// 빠른 자동 실행을 위해 시간값만 축소하며 전투 수치와 판정 경로는 변경하지 않습니다.
	void ConfigureBalanceCombatTimings();
	void RunBalanceCombatStep();
	void FinishBalanceCombatTest(bool bSucceeded, const TCHAR* Reason);

	ACWSGameMode& Owner;
	TWeakObjectPtr<ACWSEnemyBase> BalanceCombatTarget;
	TWeakObjectPtr<ACWSSupplyPickup> LastRoundSupply;
	TMap<int32, int32> BalanceShotsByRound;
	TMap<ECWSEnemyType, int32> BalanceKillsByType;
	FTimerHandle BalanceCombatTimer;
	// 아래 누적값은 최종 성공 marker에 실제 사격/보급 결과를 증명합니다.
	float BalanceCombatStartTime = 0.0f;
	int32 BalanceInitialAmmo = 0;
	int32 BalanceAmmoGained = 0;
	int32 BalanceShotsFired = 0;
	int32 BalanceMissedShots = 0;
	int32 BalanceAmmoSuppliesCollected = 0;
	int32 BalanceHealthSuppliesCollected = 0;
	// Runner 수명주기와 현재 조준 단계입니다.
	bool bBalanceCombatTest = false;
	bool bBalanceCombatConfigured = false;
	bool bBalanceCombatFinished = false;
	bool bBalanceTargetAimPrimed = false;
};
