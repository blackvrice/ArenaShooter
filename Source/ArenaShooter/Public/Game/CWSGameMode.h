#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CWSGameMode.generated.h"

class ACWSEnemyBase;
class ACWSWaveManager;

UCLASS()
class ARENASHOOTER_API ACWSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACWSGameMode();

	virtual void BeginPlay() override;

private:
	void RunRoundOneSmokeStep();
	void FinishSmokeTest(bool bSucceeded, const TCHAR* Reason);

	UFUNCTION()
	void HandleRoundCleared(int32 RoundNumber);

	TWeakObjectPtr<ACWSWaveManager> SmokeWaveManager;
	TMap<TWeakObjectPtr<ACWSEnemyBase>, FVector> SmokeEnemyStartLocations;
	FTimerHandle SmokeStepTimer;
	float SmokeStartTime = 0.0f;
	bool bSmokeTestEnabled = false;
	bool bSmokeSawPlayer = false;
	bool bSmokeSawEnemyMovement = false;
	bool bSmokeFinished = false;
};
