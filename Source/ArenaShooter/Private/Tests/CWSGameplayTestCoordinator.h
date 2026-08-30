#pragma once

#include "CoreMinimal.h"

class ACWSGameMode;
class ACWSSupplyPickup;
class FCWSBalanceTestRunner;
class FCWSCombatSmokeRunner;
class FCWSScreenshotTestRunner;
enum class ECWSWavePhase : uint8;

class FCWSGameplayTestCoordinator
{
public:
	explicit FCWSGameplayTestCoordinator(ACWSGameMode& InOwner);
	~FCWSGameplayTestCoordinator();

	bool StartFromCommandLine();
	bool ShouldKeepTitleScreen() const;
	void HandleRoundCleared(int32 RoundNumber);
	void HandleWavePhaseChanged(ECWSWavePhase WavePhase, int32 RoundNumber);
	void HandleAllRoundsCompleted();
	void HandlePlayerDeath();
	void HandleSupplySpawned(ACWSSupplyPickup* Supply);

private:
	ACWSGameMode& Owner;
	TUniquePtr<FCWSCombatSmokeRunner> CombatSmokeRunner;
	TUniquePtr<FCWSBalanceTestRunner> BalanceTestRunner;
	TUniquePtr<FCWSScreenshotTestRunner> ScreenshotTestRunner;
};
