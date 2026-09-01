#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CWSHUD.generated.h"

class ACWSWaveManager;
class ACWSBossEnemy;
class UFont;

/**
 * Widget 에셋 없이 Canvas에 현재 Production 상태를 직접 그리는 HUD입니다.
 * 매 프레임 Player 컴포넌트와 Wave/GameMode 상태를 읽기만 하며 게임 규칙은 변경하지 않습니다.
 */
UCLASS()
class ARENASHOOTER_API ACWSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	// 월드 검색은 최초 1회만 수행하고, 대상이 파괴되면 Weak 캐시를 다시 채웁니다.
	ACWSWaveManager* FindWaveManager();
	ACWSBossEnemy* FindLivingBoss();
	void DrawTitleScreen(UFont* Font, float CenterX, float CenterY);
	void DrawRoundAnnouncement(ACWSWaveManager* WaveManager, UFont* Font, float CenterX, float CenterY);
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, UFont* Font, float Scale);

	TWeakObjectPtr<ACWSWaveManager> CachedWaveManager;
	TWeakObjectPtr<ACWSBossEnemy> CachedBoss;
};
