#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"

class ACWSEnemyBase;
class ACWSGameMode;
class ACWSSupplyPickup;

class FCWSBalanceTestRunner
{
public:
	explicit FCWSBalanceTestRunner(ACWSGameMode& InOwner);
	~FCWSBalanceTestRunner();
	bool StartFromCommandLine();
	void HandleSupplySpawned(ACWSSupplyPickup* Supply);

private:
	void ConfigureBalanceCombatTimings();
	void RunBalanceCombatStep();
	void FinishBalanceCombatTest(bool bSucceeded, const TCHAR* Reason);

	ACWSGameMode& Owner;
	TWeakObjectPtr<ACWSEnemyBase> BalanceCombatTarget;
	TWeakObjectPtr<ACWSSupplyPickup> LastRoundSupply;
	TMap<int32, int32> BalanceShotsByRound;
	TMap<ECWSEnemyType, int32> BalanceKillsByType;
	FTimerHandle BalanceCombatTimer;
	float BalanceCombatStartTime = 0.0f;
	int32 BalanceInitialAmmo = 0;
	int32 BalanceAmmoGained = 0;
	int32 BalanceShotsFired = 0;
	int32 BalanceMissedShots = 0;
	int32 BalanceAmmoSuppliesCollected = 0;
	int32 BalanceHealthSuppliesCollected = 0;
	bool bBalanceCombatTest = false;
	bool bBalanceCombatConfigured = false;
	bool bBalanceCombatFinished = false;
	bool bBalanceTargetAimPrimed = false;
};
