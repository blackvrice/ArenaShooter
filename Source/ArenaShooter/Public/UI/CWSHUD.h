#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CWSHUD.generated.h"

class ACWSWaveManager;
class ACWSBossEnemy;
class UFont;

UCLASS()
class ARENASHOOTER_API ACWSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	ACWSWaveManager* FindWaveManager();
	ACWSBossEnemy* FindLivingBoss();
	void DrawTitleScreen(UFont* Font, float CenterX, float CenterY);
	void DrawRoundAnnouncement(ACWSWaveManager* WaveManager, UFont* Font, float CenterX, float CenterY);
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, UFont* Font, float Scale);

	TWeakObjectPtr<ACWSWaveManager> CachedWaveManager;
	TWeakObjectPtr<ACWSBossEnemy> CachedBoss;
};
