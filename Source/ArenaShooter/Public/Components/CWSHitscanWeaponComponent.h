#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CWSHitscanWeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSAmmoChangedEvent, int32, CurrentAmmo, int32, MaxAmmo);

UCLASS(ClassGroup = (CWS), meta = (BlueprintSpawnableComponent))
class ARENASHOOTER_API UCWSHitscanWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCWSHitscanWeaponComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMaxAmmo() const { return MaxAmmo; }

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FCWSAmmoChangedEvent OnAmmoChanged;

private:
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MaxAmmo = 60;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	int32 CurrentAmmo = 60;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "100.0"))
	float Range = 10000.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0.01"))
	float FireInterval = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bDrawDebugShot = false;

	float NextAllowedFireTime = 0.0f;
};
