#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyTypes.h"
#include "GameFramework/Character.h"
#include "CWSEnemyBase.generated.h"

class UCWSHealthComponent;
class UAnimSequenceBase;
class UPointLightComponent;
class USkeletalMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACWSEnemyBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

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

	UFUNCTION(BlueprintPure, Category = "Enemy|Feedback")
	int32 GetHitReactionCount() const { return HitReactionCount; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Feedback")
	bool IsHitReactionActive() const { return bHitReactionActive; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Feedback")
	bool HasPlayedDeathAnimation() const { return bDeathAnimationPlayed; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Feedback")
	int32 GetDeathEffectSpawnCount() const { return DeathEffectSpawnCount; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Feedback")
	int32 GetAttackAnimationCount() const { return AttackAnimationCount; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Feedback")
	int32 GetAttackSoundPlayCount() const { return AttackSoundPlayCount; }

	bool StageAttackPoseForCapture(float NormalizedTime);

	UFUNCTION(BlueprintPure, Category = "Enemy|Presentation")
	FLinearColor GetArchetypeColor() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Presentation")
	FString GetArchetypeLabel() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Presentation")
	bool HasArchetypePresentation() const { return bArchetypePresentationReady; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Presentation")
	FString GetVisualMeshPath() const;

protected:
	UFUNCTION()
	void HandleHealthChanged(
		AActor* DamagedActor,
		float CurrentHealth,
		float MaxHealth,
		AActor* ChangeInstigator);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	TObjectPtr<UAnimSequenceBase> HitReactionAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimSequenceBase> MoveAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	TObjectPtr<UAnimSequenceBase> AttackAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	TObjectPtr<UAnimSequenceBase> DeathAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Presentation")
	TObjectPtr<UStaticMeshComponent> ArchetypeMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Presentation")
	TObjectPtr<UStaticMeshComponent> ArchetypeBand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Presentation")
	TObjectPtr<UPointLightComponent> ArchetypeLight;

	bool PlayAttackAnimation();
	void ConfigureEnemyVisualProfile(
		USkeletalMesh* MeshAsset,
		UAnimSequenceBase* IdleAsset,
		UAnimSequenceBase* MoveAsset,
		UAnimSequenceBase* AttackAsset,
		UAnimSequenceBase* HitReactionAsset,
		UAnimSequenceBase* DeathAsset,
		const FVector& RelativeLocation,
		const FVector& RelativeScale);

private:
	void FinishHitReaction();
	void PlayLoopingAnimation(UAnimSequenceBase* Animation);
	bool PlayActionAnimation(UAnimSequenceBase* Animation);
	void PlayFeedbackAnimation(UAnimSequenceBase* Animation);
	void UpdateLocomotionAnimation();
	bool SpawnDeathEffect();

	float NextAllowedAttackTime = 0.0f;
	float LastObservedHealth = 0.0f;
	int32 HitReactionCount = 0;
	int32 AttackAnimationCount = 0;
	int32 AttackSoundPlayCount = 0;
	int32 DeathEffectSpawnCount = 0;
	TObjectPtr<UAnimSequenceBase> CurrentLoopingAnimation;
	float ActionAnimationEndTime = 0.0f;
	bool bHitReactionActive = false;
	bool bDeathAnimationPlayed = false;
	bool bArchetypePresentationReady = false;
	FTimerHandle HitReactionTimerHandle;
};
