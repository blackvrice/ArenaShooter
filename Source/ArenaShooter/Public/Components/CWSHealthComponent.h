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

/**
 * 액터의 체력 규칙을 한곳에서 관리하는 공용 컴포넌트입니다.
 *
 * Unreal의 ApplyDamage 계열 호출은 소유 액터의 OnTakeAnyDamage로 들어오며,
 * 여기서 실제 체력 변경과 OnHealthChanged/OnDeath 이벤트로 변환됩니다.
 * 적, 플레이어, HUD, WaveManager가 서로를 직접 참조하지 않도록 만드는 경계입니다.
 */
UCLASS(ClassGroup = (CWS), meta = (BlueprintSpawnableComponent))
class ARENASHOOTER_API UCWSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCWSHealthComponent();

	virtual void BeginPlay() override;

	/** 양수는 회복, 음수는 피해입니다. 실제로 반영된 변화량을 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyHealthChange(float Delta, AActor* ChangeInstigator = nullptr);

	/** 일반 데미지 경로를 우회하지 않고 현재 체력을 0으로 만드는 편의 함수입니다. */
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
	/** 엔진 데미지 이벤트를 이 컴포넌트의 부호 있는 체력 변화로 변환합니다. */
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
