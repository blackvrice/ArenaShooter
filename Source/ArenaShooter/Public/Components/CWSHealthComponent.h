#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CWSHealthComponent.generated.h"

class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FCWSHealthChangedEvent,
	AActor*, DamagedActor,
	float, CurrentHealth,
	float, MaxHealth,
	AActor*, ChangeInstigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSDeathEvent, AActor*, DeadActor);

UCLASS(ClassGroup = (CWS), meta = (BlueprintSpawnableComponent))
class ARENASHOOTER_API UCWSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCWSHealthComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyHealthChange(float Delta, AActor* ChangeInstigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill(AActor* ChangeInstigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth = true);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return !bIsDead && CurrentHealth > 0.0f; }

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FCWSHealthChangedEvent OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FCWSDeathEvent OnDeath;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Health")
	bool bIsDead = false;
};
