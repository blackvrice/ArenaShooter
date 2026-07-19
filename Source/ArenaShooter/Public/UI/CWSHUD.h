#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CWSHUD.generated.h"

class ACWSWaveManager;

UCLASS()
class ARENASHOOTER_API ACWSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	ACWSWaveManager* FindWaveManager();

	TWeakObjectPtr<ACWSWaveManager> CachedWaveManager;
};
