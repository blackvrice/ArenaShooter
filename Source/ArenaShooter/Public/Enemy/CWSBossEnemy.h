#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyBase.h"
#include "CWSBossEnemy.generated.h"

UENUM(BlueprintType)
enum class ECWSBossPhase : uint8
{
	PhaseOne UMETA(DisplayName = "Phase 1"),
	PhaseTwo UMETA(DisplayName = "Phase 2"),
	FinalPhase UMETA(DisplayName = "Final Phase")
};

UENUM(BlueprintType)
enum class ECWSBossPattern : uint8
{
	None UMETA(DisplayName = "None"),
	GroundSlam UMETA(DisplayName = "Ground Slam"),
	Shockwave UMETA(DisplayName = "Shockwave")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSBossPhaseEvent, ECWSBossPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCWSBossPatternEvent, ECWSBossPattern, Pattern);

UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSBossEnemy : public ACWSEnemyBase
{
	GENERATED_BODY()

public:
	ACWSBossEnemy();

	virtual void BeginPlay() override;
	virtual bool TryAttack(AActor* TargetActor) override;

	UFUNCTION(BlueprintPure, Category = "Boss")
	ECWSBossPhase GetBossPhase() const { return BossPhase; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	ECWSBossPattern GetLastPattern() const { return LastPattern; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	int32 GetPatternExecutionCount() const { return PatternExecutionCount; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	FString GetBossPhaseLabel() const;

	UFUNCTION(BlueprintPure, Category = "Boss")
	FString GetLastPatternLabel() const;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FCWSBossPhaseEvent OnBossPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FCWSBossPatternEvent OnBossPatternExecuted;

private:
	UFUNCTION()
	void HandleBossHealthChanged(
		AActor* DamagedActor,
		float CurrentHealth,
		float MaxHealth,
		AActor* ChangeInstigator);

	void UpdateBossPhase(float HealthPercent);
	bool ExecuteGroundSlam(AActor* TargetActor);
	bool ExecuteShockwave(AActor* TargetActor);
	void RecordPattern(ECWSBossPattern Pattern);

	UPROPERTY(EditAnywhere, Category = "Boss|Stats", meta = (ClampMin = "1.0"))
	float BossMaxHealth = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Patterns", meta = (ClampMin = "0.0"))
	float GroundSlamDamage = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Patterns", meta = (ClampMin = "1.0"))
	float GroundSlamRadius = 650.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Patterns", meta = (ClampMin = "0.0"))
	float ShockwaveDamage = 35.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Patterns", meta = (ClampMin = "1.0"))
	float ShockwaveRadius = 850.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Patterns", meta = (ClampMin = "0.0"))
	float ShockwaveStrength = 900.0f;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	ECWSBossPhase BossPhase = ECWSBossPhase::PhaseOne;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	ECWSBossPattern LastPattern = ECWSBossPattern::None;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	int32 PatternExecutionCount = 0;

	float NextPatternTime = 0.0f;
	bool bUseShockwaveNext = false;
};
