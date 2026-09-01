#pragma once

#include "CoreMinimal.h"

class ACWSGameMode;
class ACWSSupplyPickup;
class FCWSBalanceTestRunner;
class FCWSCombatSmokeRunner;
class FCWSScreenshotTestRunner;
enum class ECWSWavePhase : uint8;

/**
 * 명령행 플래그를 해석해 정확히 하나의 자동 검증 Runner에 실행을 위임합니다.
 * GameMode가 전달한 Production 이벤트를 활성 Runner로 중계할 뿐, 게임 상태를 복제하지 않습니다.
 */
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
	// TUniquePtr로 UObject가 아닌 Runner의 수명을 GameMode 수명에 묶습니다.
	TUniquePtr<FCWSCombatSmokeRunner> CombatSmokeRunner;
	TUniquePtr<FCWSBalanceTestRunner> BalanceTestRunner;
	TUniquePtr<FCWSScreenshotTestRunner> ScreenshotTestRunner;
};
