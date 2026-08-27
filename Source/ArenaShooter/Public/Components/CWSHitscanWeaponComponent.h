#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CWSHitscanWeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSAmmoChangedEvent, int32, CurrentAmmo, int32, MaxAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSReserveAmmoChangedEvent, int32, ReserveAmmo, int32, MaxReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSReloadStateEvent, bool, bIsReloading);

UCLASS(ClassGroup = (CWS), meta = (BlueprintSpawnableComponent))
class ARENASHOOTER_API UCWSHitscanWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCWSHitscanWeaponComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 AddReserveAmmo(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMaxAmmo() const { return MaxAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetReserveAmmo() const { return CurrentReserveAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMaxReserveAmmo() const { return MaxReserveAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetStartingReserveAmmo() const { return StartingReserveAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetDamage() const { return Damage; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetFireInterval() const { return FireInterval; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadDuration() const { return ReloadDuration; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Feedback")
	int32 GetImpactEffectSpawnCount() const { return ImpactEffectSpawnCount; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Feedback")
	int32 GetFireSoundPlayCount() const { return FireSoundPlayCount; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Feedback")
	int32 GetImpactSoundPlayCount() const { return ImpactSoundPlayCount; }

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FCWSAmmoChangedEvent OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FCWSReserveAmmoChangedEvent OnReserveAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FCWSReloadStateEvent OnReloadStateChanged;

private:
	void CompleteReload();

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MaxAmmo = 60;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	int32 CurrentAmmo = 60;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0"))
	int32 StartingReserveAmmo = 360;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MaxReserveAmmo = 480;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	int32 CurrentReserveAmmo = 360;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0.05"))
	float ReloadDuration = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "100.0"))
	float Range = 10000.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0.01"))
	float FireInterval = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bDrawDebugShot = false;

	float NextAllowedFireTime = 0.0f;
	bool bIsReloading = false;
	int32 ImpactEffectSpawnCount = 0;
	int32 FireSoundPlayCount = 0;
	int32 ImpactSoundPlayCount = 0;
	FTimerHandle ReloadTimerHandle;
};
