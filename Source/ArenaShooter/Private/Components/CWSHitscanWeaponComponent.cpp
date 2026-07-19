#include "Components/CWSHitscanWeaponComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UCWSHitscanWeaponComponent::UCWSHitscanWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCWSHitscanWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MaxAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
}

bool UCWSHitscanWeaponComponent::TryFire()
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwningPawn || !World || CurrentAmmo <= 0 || World->GetTimeSeconds() < NextAllowedFireTime)
	{
		return false;
	}

	FVector ViewLocation = OwningPawn->GetPawnViewLocation();
	FRotator ViewRotation = OwningPawn->GetViewRotation();
	if (AController* Controller = OwningPawn->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	NextAllowedFireTime = World->GetTimeSeconds() + FireInterval;
	--CurrentAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);

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

void UCWSHitscanWeaponComponent::Reload()
{
	CurrentAmmo = MaxAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
}
