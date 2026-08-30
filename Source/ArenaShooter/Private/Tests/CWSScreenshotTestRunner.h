#pragma once

#include "CoreMinimal.h"

class ACWSEnemyBase;
class ACWSGameMode;

class FCWSScreenshotTestRunner
{
public:
	explicit FCWSScreenshotTestRunner(ACWSGameMode& InOwner);
	~FCWSScreenshotTestRunner();
	bool StartFromCommandLine();
	bool IsTitleScreenshotTestEnabled() const { return bTitleScreenshotTest; }

private:
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
