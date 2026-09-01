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

/**
 * 모든 적이 공유하는 체력, 근접 공격, 애니메이션, 피드백 표현의 기반 클래스입니다.
 *
 * AIController는 추적/거리 판단만 하고 실제 공격 쿨다운과 피해 적용은 이 클래스가
 * 책임집니다. Fast/Tank/Boss는 이 공통 경로를 재사용하면서 능력치와 특수 패턴만 바꿉니다.
 */
UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACWSEnemyBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** 사거리와 월드 시간 기반 쿨다운이 모두 만족할 때 플레이어에게 피해를 줍니다. */
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

	/** Screenshot Runner가 실제 공격 애니메이션의 특정 지점을 재현할 때만 사용합니다. */
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
	// HealthComponent 이벤트를 피격/사망 표현으로 번역하는 연결 지점입니다.
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
	/** 파생 적 생성자가 에셋과 상대 Transform을 한 번에 교체하는 공통 설정 함수입니다. */
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

	// Tick 간격이나 프레임률과 무관하게 공격 주기를 유지하기 위한 절대 월드 시각입니다.
	float NextAllowedAttackTime = 0.0f;
	float LastObservedHealth = 0.0f;
	int32 HitReactionCount = 0;
	int32 AttackAnimationCount = 0;
	int32 AttackSoundPlayCount = 0;
	int32 DeathEffectSpawnCount = 0;
	TObjectPtr<UAnimSequenceBase> CurrentLoopingAnimation;
	// 피격/공격/사망 애니메이션이 locomotion에 즉시 덮이지 않도록 보호합니다.
	float ActionAnimationEndTime = 0.0f;
	bool bHitReactionActive = false;
	bool bDeathAnimationPlayed = false;
	bool bArchetypePresentationReady = false;
	FTimerHandle HitReactionTimerHandle;
};
