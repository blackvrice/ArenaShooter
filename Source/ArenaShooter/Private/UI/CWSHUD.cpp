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
			FString::Printf(
				TEXT("AMMO    %d / %d   |   RESERVE %d / %d%s"),
				Weapon->GetCurrentAmmo(),
				Weapon->GetMaxAmmo(),
				Weapon->GetReserveAmmo(),
				Weapon->GetMaxReserveAmmo(),
				Weapon->IsReloading() ? TEXT("   |   RELOADING") : TEXT("")),
			Weapon->IsReloading() ? FLinearColor::Yellow : FLinearColor::White,
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
	DrawRoundAnnouncement(WaveManager, Font, CenterX, CenterY);

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

void ACWSHUD::DrawRoundAnnouncement(
	ACWSWaveManager* WaveManager,
	UFont* Font,
	const float CenterX,
	const float CenterY)
{
	if (!WaveManager || !Font)
	{
		return;
	}

	FString Title;
	FString Subtitle;
	FLinearColor AccentColor(1.0f, 0.72f, 0.12f);
	const int32 RoundNumber = WaveManager->GetCurrentRound();
	const int32 RemainingSeconds = FMath::Max(FMath::CeilToInt(WaveManager->GetPhaseTimeRemaining()), 1);

	switch (WaveManager->GetWavePhase())
	{
	case ECWSWavePhase::Preparing:
		Title = RoundNumber >= 5 ? TEXT("FINAL ROUND") : FString::Printf(TEXT("ROUND %d"), RoundNumber);
		Subtitle = RoundNumber >= 5
			? FString::Printf(TEXT("BOSS IN %d"), RemainingSeconds)
			: FString::Printf(TEXT("STARTS IN %d"), RemainingSeconds);
		break;
	case ECWSWavePhase::Active:
		if (WaveManager->GetPhaseElapsedTime() > 1.75f)
		{
			return;
		}
		Title = RoundNumber >= 5 ? TEXT("BOSS ROUND") : FString::Printf(TEXT("ROUND %d START"), RoundNumber);
		Subtitle = RoundNumber >= 5 ? TEXT("ELIMINATE THE BOSS") : TEXT("SURVIVE THE WAVE");
		break;
	case ECWSWavePhase::RoundCleared:
		Title = FString::Printf(TEXT("ROUND %d CLEAR"), RoundNumber);
		Subtitle = FString::Printf(TEXT("NEXT ROUND IN %d"), RemainingSeconds);
		AccentColor = FLinearColor(0.15f, 0.9f, 0.35f);
		break;
	default:
		return;
	}

	const float PanelWidth = FMath::Min(Canvas->ClipX * 0.52f, 620.0f);
	const float PanelHeight = 112.0f;
	const float PanelX = CenterX - PanelWidth * 0.5f;
	const float PanelY = CenterY - 205.0f;
	DrawRect(FLinearColor(0.015f, 0.02f, 0.035f, 0.86f), PanelX, PanelY, PanelWidth, PanelHeight);
	DrawRect(AccentColor, PanelX, PanelY, PanelWidth, 4.0f);
	DrawCenteredText(Title, AccentColor, CenterX, PanelY + 20.0f, Font, 1.65f);
	DrawCenteredText(Subtitle, FLinearColor::White, CenterX, PanelY + 69.0f, GEngine->GetSmallFont(), 1.05f);
}

void ACWSHUD::DrawCenteredText(
	const FString& Text,
	const FLinearColor& Color,
	const float CenterX,
	const float Y,
	UFont* Font,
	const float Scale)
{
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);
	DrawText(Text, Color, CenterX - TextWidth * 0.5f, Y, Font, Scale, false);
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
