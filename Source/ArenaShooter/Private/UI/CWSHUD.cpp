#include "UI/CWSHUD.h"

#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Enemy/CWSBossEnemy.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Wave/CWSWaveManager.h"

void ACWSHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !GEngine)
	{
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	DrawLine(CenterX - 8.0f, CenterY, CenterX + 8.0f, CenterY, FLinearColor::White, 1.5f);
	DrawLine(CenterX, CenterY - 8.0f, CenterX, CenterY + 8.0f, FLinearColor::White, 1.5f);

	APlayerController* PlayerController = GetOwningPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UCWSHealthComponent* Health = PlayerPawn ? PlayerPawn->FindComponentByClass<UCWSHealthComponent>() : nullptr;
	const UCWSHitscanWeaponComponent* Weapon =
		PlayerPawn ? PlayerPawn->FindComponentByClass<UCWSHitscanWeaponComponent>() : nullptr;
	ACWSWaveManager* WaveManager = FindWaveManager();
	ACWSBossEnemy* Boss = FindLivingBoss();

	float TextY = 35.0f;
	UFont* Font = GEngine->GetMediumFont();
	if (Health)
	{
		DrawText(
			FString::Printf(TEXT("HEALTH  %.0f / %.0f"), Health->GetCurrentHealth(), Health->GetMaxHealth()),
			Health->IsAlive() ? FLinearColor::White : FLinearColor::Red,
			35.0f,
			TextY,
			Font,
			1.0f,
			false);
		TextY += 28.0f;
	}
	if (Weapon)
	{
		DrawText(
			FString::Printf(TEXT("AMMO    %d / %d"), Weapon->GetCurrentAmmo(), Weapon->GetMaxAmmo()),
			FLinearColor::White,
			35.0f,
			TextY,
			Font,
			1.0f,
			false);
		TextY += 28.0f;
	}
	if (WaveManager)
	{
		DrawText(
			FString::Printf(TEXT("ROUND   %d / 5"), WaveManager->GetCurrentRound()),
			FLinearColor::White,
			35.0f,
			TextY,
			Font,
			1.0f,
			false);
		TextY += 28.0f;
		DrawText(
			FString::Printf(TEXT("ENEMIES %d"), WaveManager->GetRemainingEnemyCount()),
			FLinearColor::White,
			35.0f,
			TextY,
			Font,
			1.0f,
			false);
	}

	if (Boss)
	{
		const UCWSHealthComponent* BossHealth = Boss->GetHealthComponent();
		if (BossHealth && BossHealth->IsAlive())
		{
			const float BarWidth = FMath::Min(Canvas->ClipX * 0.45f, 560.0f);
			const float BarHeight = 18.0f;
			const float BarX = CenterX - BarWidth * 0.5f;
			const float BarY = 38.0f;
			DrawRect(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f), BarX - 2.0f, BarY - 2.0f, BarWidth + 4.0f, BarHeight + 4.0f);
			DrawRect(FLinearColor(0.75f, 0.04f, 0.04f), BarX, BarY, BarWidth * BossHealth->GetHealthPercent(), BarHeight);
			DrawText(
				FString::Printf(
					TEXT("BOSS  %.0f / %.0f  |  %s  |  %s"),
					BossHealth->GetCurrentHealth(),
					BossHealth->GetMaxHealth(),
					*Boss->GetBossPhaseLabel(),
					*Boss->GetLastPatternLabel()),
				FLinearColor::White,
				BarX,
				BarY + 23.0f,
				GEngine->GetSmallFont(),
				1.0f,
				false);
		}
	}

	DrawText(
		TEXT("LMB: FIRE   RMB: RELOAD   WASD: MOVE"),
		FLinearColor(0.7f, 0.7f, 0.7f),
		35.0f,
		Canvas->ClipY - 45.0f,
		GEngine->GetSmallFont(),
		1.0f,
		false);

	const ACWSGameMode* GameMode = GetWorld()->GetAuthGameMode<ACWSGameMode>();
	if ((GameMode && GameMode->IsGameOver()) || (Health && !Health->IsAlive()))
	{
		DrawText(TEXT("GAME OVER"), FLinearColor::Red, CenterX - 95.0f, CenterY - 70.0f, Font, 1.8f, false);
		DrawText(TEXT("PRESS ENTER TO RESTART"), FLinearColor::White, CenterX - 120.0f, CenterY - 30.0f, Font, 1.0f, false);
	}
	else if ((GameMode && GameMode->IsGameCleared()) || (WaveManager && WaveManager->bAllRoundsCompleted))
	{
		DrawText(TEXT("ARENA CLEARED"), FLinearColor::Green, CenterX - 130.0f, CenterY - 70.0f, Font, 1.8f, false);
		DrawText(TEXT("PRESS ENTER TO RESTART"), FLinearColor::White, CenterX - 120.0f, CenterY - 30.0f, Font, 1.0f, false);
	}
}

ACWSWaveManager* ACWSHUD::FindWaveManager()
{
	if (CachedWaveManager.IsValid())
	{
		return CachedWaveManager.Get();
	}

	for (TActorIterator<ACWSWaveManager> It(GetWorld()); It; ++It)
	{
		CachedWaveManager = *It;
		return *It;
	}
	return nullptr;
}

ACWSBossEnemy* ACWSHUD::FindLivingBoss()
{
	if (CachedBoss.IsValid() && CachedBoss->GetHealthComponent()->IsAlive())
	{
		return CachedBoss.Get();
	}

	CachedBoss.Reset();
	for (TActorIterator<ACWSBossEnemy> It(GetWorld()); It; ++It)
	{
		if (It->GetHealthComponent()->IsAlive())
		{
			CachedBoss = *It;
			return *It;
		}
	}
	return nullptr;
}
