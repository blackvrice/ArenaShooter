#include "Components/CWSHitscanWeaponComponent.h"

#include "Audio/CWSCombatSound.h"
#include "DrawDebugHelpers.h"
#include "Feedback/CWSCombatBurstEffect.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UCWSHitscanWeaponComponent::UCWSHitscanWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCWSHitscanWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MaxAmmo;
	CurrentReserveAmmo = FMath::Clamp(StartingReserveAmmo, 0, MaxReserveAmmo);
	bIsReloading = false;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
	OnReserveAmmoChanged.Broadcast(CurrentReserveAmmo, MaxReserveAmmo);
}

void UCWSHitscanWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

bool UCWSHitscanWeaponComponent::TryFire()
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwningPawn || !World || bIsReloading || World->GetTimeSeconds() < NextAllowedFireTime)
	{
		return false;
	}
	if (CurrentAmmo <= 0)
	{
		Reload();
		return false;
	}

	// 총구가 아닌 카메라/Controller 시점을 사용해 화면 조준점과 LineTrace를 일치시킨다.
	FVector ViewLocation = OwningPawn->GetPawnViewLocation();
	FRotator ViewRotation = OwningPawn->GetViewRotation();
	if (AController* Controller = OwningPawn->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	// 탄약 차감과 피드백은 명중 여부와 관계없이 유효한 한 발이 발사된 시점에 처리한다.
	NextAllowedFireTime = World->GetTimeSeconds() + FireInterval;
	--CurrentAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
	if (PlayCWSCombatSound(this, ViewLocation, ECWSCombatSoundType::WeaponFire, 0.85f))
	{
		++FireSoundPlayCount;
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * Range;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CWSWeaponTrace), true, OwningPawn);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	const FVector FinalPoint = bHit ? Hit.ImpactPoint : TraceEnd;

	if (bDrawDebugShot)
	{
		DrawDebugLine(World, ViewLocation, FinalPoint, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.5f);
	}

	if (bHit && IsValid(Hit.GetActor()))
	{
		// 시각/음향 피드백을 먼저 만들고 Unreal의 표준 PointDamage 이벤트로 피해를 전달한다.
		// HealthComponent가 이 이벤트를 받아 체력/사망 delegate로 변환한다.
		if (PlayCWSCombatSound(this, Hit.ImpactPoint, ECWSCombatSoundType::BulletImpact, 0.7f))
		{
			++ImpactSoundPlayCount;
		}
		if (ACWSCombatBurstEffect::SpawnBurst(
			World,
			Hit.ImpactPoint + Hit.ImpactNormal * 8.0f,
			FLinearColor(0.15f, 0.8f, 1.0f),
			0.32f))
		{
			++ImpactEffectSpawnCount;
		}
		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(),
			Damage,
			ViewRotation.Vector(),
			Hit,
			OwningPawn->GetController(),
			OwningPawn,
			UDamageType::StaticClass());
	}

	return true;
}

bool UCWSHitscanWeaponComponent::Reload()
{
	UWorld* World = GetWorld();
	if (!World || bIsReloading || CurrentAmmo >= MaxAmmo || CurrentReserveAmmo <= 0)
	{
		return false;
	}

	bIsReloading = true;
	OnReloadStateChanged.Broadcast(true);
	World->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UCWSHitscanWeaponComponent::CompleteReload,
		ReloadDuration,
		false);
	return true;
}

int32 UCWSHitscanWeaponComponent::AddReserveAmmo(const int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}
	const int32 OldReserveAmmo = CurrentReserveAmmo;
	CurrentReserveAmmo = FMath::Clamp(CurrentReserveAmmo + Amount, 0, MaxReserveAmmo);
	const int32 AddedAmmo = CurrentReserveAmmo - OldReserveAmmo;
	if (AddedAmmo > 0)
	{
		OnReserveAmmoChanged.Broadcast(CurrentReserveAmmo, MaxReserveAmmo);
	}
	return AddedAmmo;
}

void UCWSHitscanWeaponComponent::CompleteReload()
{
	// 예비 탄약이 부족한 경우에도 가능한 만큼만 옮기며 총 탄약량은 보존한다.
	const int32 MissingAmmo = FMath::Max(MaxAmmo - CurrentAmmo, 0);
	const int32 LoadedAmmo = FMath::Min(MissingAmmo, CurrentReserveAmmo);
	CurrentAmmo += LoadedAmmo;
	CurrentReserveAmmo -= LoadedAmmo;
	bIsReloading = false;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
	OnReserveAmmoChanged.Broadcast(CurrentReserveAmmo, MaxReserveAmmo);
	OnReloadStateChanged.Broadcast(false);
}
