#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CWSEnemyBase.generated.h"

class UCWSHealthComponent;

UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACWSEnemyBase();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool TryAttack(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UCWSHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UCWSHealthComponent> HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float AttackRange = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.05"))
	float AttackInterval = 1.0f;

private:
	float NextAllowedAttackTime = 0.0f;
};
