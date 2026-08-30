#include "Tests/CWSGameplayTestCoordinator.h"

#include "Tests/CWSBalanceTestRunner.h"
#include "Tests/CWSCombatSmokeRunner.h"
#include "Tests/CWSScreenshotTestRunner.h"

FCWSGameplayTestCoordinator::FCWSGameplayTestCoordinator(ACWSGameMode& InOwner)
	: Owner(InOwner)
{
}

FCWSGameplayTestCoordinator::~FCWSGameplayTestCoordinator() = default;

bool FCWSGameplayTestCoordinator::StartFromCommandLine()
{
	bool bAnyTestStarted = false;
	CombatSmokeRunner = MakeUnique<FCWSCombatSmokeRunner>(Owner);
	if (!CombatSmokeRunner->StartFromCommandLine())
	{
		CombatSmokeRunner.Reset();
	}
	else
	{
		bAnyTestStarted = true;
	}

	BalanceTestRunner = MakeUnique<FCWSBalanceTestRunner>(Owner);
	if (!BalanceTestRunner->StartFromCommandLine())
	{
		BalanceTestRunner.Reset();
	}
	else
	{
		bAnyTestStarted = true;
	}

	ScreenshotTestRunner = MakeUnique<FCWSScreenshotTestRunner>(Owner);
	if (!ScreenshotTestRunner->StartFromCommandLine())
	{
		ScreenshotTestRunner.Reset();
	}
	else
	{
		bAnyTestStarted = true;
	}
	return bAnyTestStarted;
}

bool FCWSGameplayTestCoordinator::ShouldKeepTitleScreen() const
{
	return ScreenshotTestRunner && ScreenshotTestRunner->IsTitleScreenshotTestEnabled();
}

void FCWSGameplayTestCoordinator::HandleRoundCleared(const int32 RoundNumber)
{
	if (CombatSmokeRunner)
	{
		CombatSmokeRunner->HandleRoundCleared(RoundNumber);
	}
}

void FCWSGameplayTestCoordinator::HandleWavePhaseChanged(
	const ECWSWavePhase WavePhase,
	const int32 RoundNumber)
{
	if (CombatSmokeRunner)
	{
		CombatSmokeRunner->HandleWavePhaseChanged(WavePhase, RoundNumber);
	}
}

void FCWSGameplayTestCoordinator::HandleAllRoundsCompleted()
{
	if (CombatSmokeRunner)
	{
		CombatSmokeRunner->HandleAllRoundsCompleted();
	}
}

void FCWSGameplayTestCoordinator::HandlePlayerDeath()
{
	if (CombatSmokeRunner)
	{
		CombatSmokeRunner->HandlePlayerDeath();
	}
}

void FCWSGameplayTestCoordinator::HandleSupplySpawned(ACWSSupplyPickup* Supply)
{
	if (CombatSmokeRunner)
	{
		CombatSmokeRunner->HandleSupplySpawned(Supply);
	}
	if (BalanceTestRunner)
	{
		BalanceTestRunner->HandleSupplySpawned(Supply);
	}
}
