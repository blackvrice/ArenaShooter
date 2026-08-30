#include "Tests/CWSBalanceTestRunner.h"

#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Enemy/CWSEnemyBase.h"
#include "EngineUtils.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/CWSSupplyPickup.h"
#include "Player/CWSPlayerCharacter.h"
#include "TimerManager.h"
#include "Wave/CWSWaveManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSBalanceTest, Log, All);

FCWSBalanceTestRunner::FCWSBalanceTestRunner(ACWSGameMode& InOwner)
    : Owner(InOwner)
{
}

FCWSBalanceTestRunner::~FCWSBalanceTestRunner() = default;

bool FCWSBalanceTestRunner::StartFromCommandLine()
{
    bBalanceCombatTest = FParse::Param(FCommandLine::Get(), TEXT("CWSBalanceCombatTest"));
    if (!bBalanceCombatTest)
    {
        return false;
    }

    BalanceCombatStartTime = Owner.GetWorld()->GetTimeSeconds();
    Owner.GetWorldTimerManager().SetTimer(
        BalanceCombatTimer,
		FTimerDelegate::CreateWeakLambda(&Owner, [this]() { RunBalanceCombatStep(); }),
        0.03f,
        true,
        0.1f);
    UE_LOG(LogCWSBalanceTest, Display, TEXT("CWS_BALANCE_COMBAT_STARTED"));
    return true;
}

void FCWSBalanceTestRunner::HandleSupplySpawned(ACWSSupplyPickup* Supply)
{
    LastRoundSupply = Supply;
}
void FCWSBalanceTestRunner::ConfigureBalanceCombatTimings()
{
	if (bBalanceCombatConfigured || !Owner.WaveManager.IsValid())
	{
		return;
	}

	for (FCWSRoundDefinition& Round : Owner.WaveManager->Rounds)
	{
		Round.PreRoundDelay = 0.05f;
		Round.PostRoundDelay = 0.05f;
		for (FCWSRoundSpawnGroup& Group : Round.SpawnGroups)
		{
			Group.SpawnInterval = 0.05f;
		}
	}
	bBalanceCombatConfigured = true;
}

void FCWSBalanceTestRunner::RunBalanceCombatStep()
{
	if (!bBalanceCombatTest || bBalanceCombatFinished || !Owner.GetWorld())
	{
		return;
	}
	if (Owner.GetWorld()->GetTimeSeconds() - BalanceCombatStartTime > 120.0f)
	{
		FinishBalanceCombatTest(false, TEXT("actual-hit balance run exceeded 120 seconds"));
		return;
	}

	Owner.BindGameplayActors();
	ConfigureBalanceCombatTimings();
	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(&Owner, 0));
	APlayerController* PlayerController = PlayerCharacter ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponComponent() : nullptr;
	if (!PlayerCharacter || !PlayerController || !Weapon || !Owner.WaveManager.IsValid() || !bBalanceCombatConfigured)
	{
		return;
	}
	if (BalanceInitialAmmo == 0)
	{
		BalanceInitialAmmo = Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo();
		UE_LOG(
			LogCWSBalanceTest,
			Display,
			TEXT("CWS_BALANCE_AMMO_BUDGET: Magazine=%d StartingReserve=%d MaxReserve=%d Supply=%d"),
			Weapon->GetMaxAmmo(),
			Weapon->GetStartingReserveAmmo(),
			Weapon->GetMaxReserveAmmo(),
			GetDefault<ACWSSupplyPickup>()->GetAmmoAmount());
	}

	if (ACWSSupplyPickup* Supply = LastRoundSupply.Get())
	{
		const ECWSSupplyType SupplyType = Supply->GetSupplyType();
		if (SupplyType == ECWSSupplyType::Health && Owner.PlayerHealth.IsValid())
		{
			Owner.PlayerHealth->ApplyHealthChange(-10.0f, &Owner);
		}
		const int32 AmmoBefore = Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo();
		if (Supply->TryCollect(PlayerCharacter))
		{
			if (SupplyType == ECWSSupplyType::Ammo)
			{
				++BalanceAmmoSuppliesCollected;
				BalanceAmmoGained += Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo() - AmmoBefore;
			}
			else
			{
				++BalanceHealthSuppliesCollected;
			}
			LastRoundSupply.Reset();
		}
	}
	if (Owner.PlayerHealth.IsValid())
	{
		Owner.PlayerHealth->ApplyHealthChange(Owner.PlayerHealth->GetMaxHealth(), &Owner);
	}

	TArray<ACWSEnemyBase*> AliveEnemies;
	for (TActorIterator<ACWSEnemyBase> It(Owner.GetWorld()); It; ++It)
	{
		ACWSEnemyBase* Enemy = *It;
		if (Enemy->GetHealthComponent() && Enemy->GetHealthComponent()->IsAlive())
		{
			Enemy->GetCharacterMovement()->DisableMovement();
			Enemy->DetachFromControllerPendingDestroy();
			AliveEnemies.Add(Enemy);
		}
	}

	ACWSEnemyBase* Target = BalanceCombatTarget.Get();
	if (!Target || !Target->GetHealthComponent() || !Target->GetHealthComponent()->IsAlive())
	{
		BalanceCombatTarget.Reset();
		bBalanceTargetAimPrimed = false;
		Target = AliveEnemies.IsEmpty() ? nullptr : AliveEnemies[0];
		if (Target)
		{
			BalanceCombatTarget = Target;
		}
	}

	const FVector Forward = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	int32 ParkedIndex = 0;
	for (ACWSEnemyBase* Enemy : AliveEnemies)
	{
		if (Enemy == Target)
		{
			continue;
		}
		const float Side = static_cast<float>((ParkedIndex % 9) - 4) * 180.0f;
		const float Back = 1400.0f + static_cast<float>(ParkedIndex / 9) * 180.0f;
		Enemy->TeleportTo(PlayerCharacter->GetActorLocation() - Forward * Back + Right * Side, Enemy->GetActorRotation());
		++ParkedIndex;
	}

	if (Target)
	{
		FVector CameraViewLocation;
		FRotator CameraViewRotation;
		PlayerController->GetPlayerViewPoint(CameraViewLocation, CameraViewRotation);
		const FVector ShotDirection = CameraViewRotation.Vector().GetSafeNormal();
		const FVector TargetLocation = CameraViewLocation + ShotDirection * 800.0f;
		Target->SetActorLocationAndRotation(
			TargetLocation,
			(-ShotDirection).Rotation(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		if (!bBalanceTargetAimPrimed)
		{
			bBalanceTargetAimPrimed = true;
			return;
		}
		const float HealthBefore = Target->GetHealthComponent()->GetCurrentHealth();
		const int32 AmmoBefore = Weapon->GetCurrentAmmo();
		if (Weapon->TryFire() && Weapon->GetCurrentAmmo() < AmmoBefore)
		{
			++BalanceShotsFired;
			BalanceShotsByRound.FindOrAdd(Owner.WaveManager->GetCurrentRound())++;
			const float HealthAfter = Target->GetHealthComponent()->GetCurrentHealth();
			if (HealthAfter >= HealthBefore)
			{
				++BalanceMissedShots;
			}
			if (!Target->GetHealthComponent()->IsAlive())
			{
				BalanceKillsByType.FindOrAdd(Target->GetEnemyType())++;
				BalanceCombatTarget.Reset();
				bBalanceTargetAimPrimed = false;
			}
		}
		else if (Weapon->GetCurrentAmmo() <= 0 && Weapon->GetReserveAmmo() <= 0)
		{
			FinishBalanceCombatTest(false, TEXT("ammo exhausted before all enemies were defeated"));
		}
		return;
	}

	if (Owner.bGameCleared && Owner.WaveManager->GetWavePhase() == ECWSWavePhase::Completed)
	{
		FinishBalanceCombatTest(true, TEXT("all five rounds cleared through actual hitscan damage"));
	}
}

void FCWSBalanceTestRunner::FinishBalanceCombatTest(const bool bSucceeded, const TCHAR* Reason)
{
	if (bBalanceCombatFinished)
	{
		return;
	}
	bBalanceCombatFinished = true;
	Owner.GetWorldTimerManager().ClearTimer(BalanceCombatTimer);

	ACWSPlayerCharacter* PlayerCharacter = Cast<ACWSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(&Owner, 0));
	UCWSHitscanWeaponComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponComponent() : nullptr;
	const TArray<int32> ExpectedRoundShots = {24, 40, 104, 132, 104};
	bool bRoundShotsMatch = true;
	for (int32 Index = 0; Index < ExpectedRoundShots.Num(); ++Index)
	{
		const int32 RoundNumber = Index + 1;
		const int32 ActualShots = BalanceShotsByRound.FindRef(RoundNumber);
		bRoundShotsMatch = bRoundShotsMatch && ActualShots == ExpectedRoundShots[Index];
		UE_LOG(
			LogCWSBalanceTest,
			Display,
			TEXT("CWS_BALANCE_ROUND: Round=%d ActualHitShots=%d Expected=%d"),
			RoundNumber,
			ActualShots,
			ExpectedRoundShots[Index]);
	}

	const int32 TotalEnemies = BalanceKillsByType.FindRef(ECWSEnemyType::Normal) +
		BalanceKillsByType.FindRef(ECWSEnemyType::Fast) +
		BalanceKillsByType.FindRef(ECWSEnemyType::Tank) +
		BalanceKillsByType.FindRef(ECWSEnemyType::Boss);
	const int32 RequiredAtSeventyPercent = FMath::CeilToInt(static_cast<float>(BalanceShotsFired) / 0.70f);
	const int32 TotalAvailableAmmo = BalanceInitialAmmo + 2 * GetDefault<ACWSSupplyPickup>()->GetAmmoAmount();
	const int32 RemainingAmmo = Weapon ? Weapon->GetCurrentAmmo() + Weapon->GetReserveAmmo() : -1;
	const bool bVerified = bSucceeded && bRoundShotsMatch && BalanceShotsFired == 404 && BalanceMissedShots == 0 &&
		TotalEnemies == 97 && BalanceKillsByType.FindRef(ECWSEnemyType::Normal) == 44 &&
		BalanceKillsByType.FindRef(ECWSEnemyType::Fast) == 32 && BalanceKillsByType.FindRef(ECWSEnemyType::Tank) == 20 &&
		BalanceKillsByType.FindRef(ECWSEnemyType::Boss) == 1 && BalanceAmmoSuppliesCollected == 2 &&
		BalanceHealthSuppliesCollected == 2 && BalanceAmmoGained == 180 &&
		TotalAvailableAmmo >= RequiredAtSeventyPercent && RemainingAmmo == 196;
	if (bVerified)
	{
		UE_LOG(
			LogCWSBalanceTest,
			Display,
			TEXT("CWS_BALANCE_COMBAT_SUCCESS: Enemies=97 ActualHitShots=404 AvailableAt70Percent=600 RequiredAt70Percent=%d RemainingAfterPerfectRun=%d AmmoSupplies=2 HealthSupplies=2"),
			RequiredAtSeventyPercent,
			RemainingAmmo);
	}
	else
	{
		UE_LOG(
			LogCWSBalanceTest,
			Error,
			TEXT("CWS_BALANCE_COMBAT_FAILURE: %s Enemies=%d Shots=%d Misses=%d AmmoGained=%d Remaining=%d AmmoSupplies=%d HealthSupplies=%d"),
			Reason,
			TotalEnemies,
			BalanceShotsFired,
			BalanceMissedShots,
			BalanceAmmoGained,
			RemainingAmmo,
			BalanceAmmoSuppliesCollected,
			BalanceHealthSuppliesCollected);
	}
	FPlatformMisc::RequestExitWithStatus(
		true,
		bVerified ? 0 : 21,
		bVerified ? TEXT("CWS balance combat test succeeded") : TEXT("CWS balance combat test failed"));
}
