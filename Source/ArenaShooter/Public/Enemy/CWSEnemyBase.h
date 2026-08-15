#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"
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
	virtual bool TryAttack(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UCWSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	ECWSEnemyType GetEnemyType() const { return EnemyType; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetAttackDamage() const { return AttackDamage; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetAttackInterval() const { return AttackInterval; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetMoveSpeed() const;

protected:
	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UCWSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	ECWSEnemyType EnemyType = ECWSEnemyType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float AttackRange = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.05"))
	float AttackInterval = 1.0f;

private:
	float NextAllowedAttackTime = 0.0f;
};
