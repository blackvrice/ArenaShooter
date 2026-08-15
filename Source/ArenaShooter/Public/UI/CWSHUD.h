#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CWSHUD.generated.h"

class ACWSWaveManager;
class ACWSBossEnemy;

UCLASS()
class ARENASHOOTER_API ACWSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	ACWSWaveManager* FindWaveManager();
	ACWSBossEnemy* FindLivingBoss();

	TWeakObjectPtr<ACWSWaveManager> CachedWaveManager;
	TWeakObjectPtr<ACWSBossEnemy> CachedBoss;
};
