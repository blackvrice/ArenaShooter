#pragma once

#include "CoreMinimal.h"

class ACWSEnemyBase;
class ACWSGameMode;

/**
 * Title, HUD, 전투 피드백, 공격 자세, Arena 표현을 오프스크린 캡처로 검증합니다.
 * 각 시나리오는 월드 상태를 준비한 뒤 PNG signature와 IEND 기록까지 확인하고 종료합니다.
 */
class FCWSScreenshotTestRunner
{
public:
	explicit FCWSScreenshotTestRunner(ACWSGameMode& InOwner);
	~FCWSScreenshotTestRunner();
	bool StartFromCommandLine();
	bool IsTitleScreenshotTestEnabled() const { return bTitleScreenshotTest; }

private:
	// 각 캡처는 준비/요청 단계와 디스크 기록 완료 단계로 나뉩니다.
	void RunTitleScreenshotStep();
	void FinishTitleScreenshotTest();
	void RunHudScreenshotStep();
	void FinishHudScreenshotTest();
	void RunCombatFeedbackScreenshotStep();
	void FinishCombatFeedbackScreenshotTest();
	void RunAttackFeedbackScreenshotStep();
	void FinishAttackFeedbackScreenshotTest();
	void RunVisualPolishScreenshotStep();
	void FinishVisualPolishScreenshotTest();

	ACWSGameMode& Owner;
	TWeakObjectPtr<ACWSEnemyBase> CombatFeedbackScreenshotTarget;
	TWeakObjectPtr<ACWSEnemyBase> AttackFeedbackScreenshotTarget;
	TArray<TWeakObjectPtr<ACWSEnemyBase>> VisualPolishScreenshotTargets;
	// 시나리오별 타이머를 분리해 한 테스트의 종료 타이머가 다른 캡처를 건드리지 않습니다.
	FTimerHandle TitleScreenshotTimer;
	FTimerHandle TitleScreenshotExitTimer;
	FTimerHandle HudScreenshotTimer;
	FTimerHandle HudScreenshotExitTimer;
	FTimerHandle CombatFeedbackScreenshotTimer;
	FTimerHandle CombatFeedbackScreenshotExitTimer;
	FTimerHandle AttackFeedbackScreenshotTimer;
	FTimerHandle AttackFeedbackScreenshotExitTimer;
	FTimerHandle VisualPolishScreenshotTimer;
	FTimerHandle VisualPolishScreenshotExitTimer;
	// 시작/요청 시각은 월드 준비 timeout과 파일 기록 timeout을 구분합니다.
	float TitleScreenshotStartTime = 0.0f;
	float TitleScreenshotRequestTime = 0.0f;
	float HudScreenshotStartTime = 0.0f;
	float HudScreenshotRequestTime = 0.0f;
	float CombatFeedbackScreenshotStartTime = 0.0f;
	float CombatFeedbackScreenshotRequestTime = 0.0f;
	float AttackFeedbackScreenshotStartTime = 0.0f;
	float AttackFeedbackScreenshotRequestTime = 0.0f;
	float VisualPolishScreenshotStartTime = 0.0f;
	float VisualPolishScreenshotRequestTime = 0.0f;
	FString TitleScreenshotPath;
	FString HudScreenshotPath;
	FString CombatFeedbackScreenshotPath;
	FString AttackFeedbackScreenshotPath;
	FString VisualPolishScreenshotPath;
	int32 CombatFeedbackCaptureDelaySteps = 0;
	int32 AttackFeedbackCaptureDelaySteps = 0;
	int32 VisualPolishCaptureDelaySteps = 0;
	// 아래 플래그는 각 캡처 상태 머신의 현재 단계와 확인된 계약입니다.
	bool bTitleScreenshotTest = false;
	bool bTitleScreenshotRequested = false;
	bool bTitleStartTriggered = false;
	bool bHudScreenshotTest = false;
	bool bHudScreenshotRequested = false;
	bool bHudSawArenaVisuals = false;
	bool bHudLoggedArenaPresentation = false;
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
