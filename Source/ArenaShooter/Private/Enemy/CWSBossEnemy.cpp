#include "Enemy/CWSBossEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSBoss, Log, All);

ACWSBossEnemy::ACWSBossEnemy()
{
	EnemyType = ECWSEnemyType::Boss;
	HealthComponent->SetMaxHealth(BossMaxHealth);
	GetCharacterMovement()->MaxWalkSpeed = 260.0f;
	AttackDamage = GroundSlamDamage;
	AttackRange = GroundSlamRadius;
	AttackInterval = 2.4f;
	GetCapsuleComponent()->InitCapsuleSize(60.0f, 130.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -130.0f));
	GetMesh()->SetRelativeScale3D(FVector(1.35f));
}

void ACWSBossEnemy::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ACWSBossEnemy::HandleBossHealthChanged);
	UpdateBossPhase(HealthComponent->GetHealthPercent());
	UE_LOG(LogCWSBoss, Display, TEXT("Boss spawned with %.0f health."), HealthComponent->GetMaxHealth());
}

bool ACWSBossEnemy::TryAttack(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetActor) || !HealthComponent->IsAlive() || World->GetTimeSeconds() < NextPatternTime)
	{
		return false;
	}

	ECWSBossPattern Pattern = ECWSBossPattern::GroundSlam;
	if (BossPhase == ECWSBossPhase::FinalPhase || (BossPhase == ECWSBossPhase::PhaseTwo && bUseShockwaveNext))
	{
		Pattern = ECWSBossPattern::Shockwave;
	}

	const bool bExecuted = Pattern == ECWSBossPattern::Shockwave
		? ExecuteShockwave(TargetActor)
		: ExecuteGroundSlam(TargetActor);
	if (!bExecuted)
	{
		return false;
	}

	if (BossPhase == ECWSBossPhase::PhaseTwo)
	{
		bUseShockwaveNext = !bUseShockwaveNext;
	}
	const float PhaseInterval = BossPhase == ECWSBossPhase::PhaseOne
		? 2.4f
		: BossPhase == ECWSBossPhase::PhaseTwo ? 1.8f : 1.1f;
	NextPatternTime = World->GetTimeSeconds() + PhaseInterval;
	RecordPattern(Pattern);
	return true;
}

FString ACWSBossEnemy::GetBossPhaseLabel() const
{
	switch (BossPhase)
	{
	case ECWSBossPhase::PhaseTwo:
		return TEXT("PHASE 2");
	case ECWSBossPhase::FinalPhase:
		return TEXT("FINAL PHASE");
	default:
		return TEXT("PHASE 1");
	}
}

FString ACWSBossEnemy::GetLastPatternLabel() const
{
	switch (LastPattern)
	{
	case ECWSBossPattern::GroundSlam:
		return TEXT("GROUND SLAM");
	case ECWSBossPattern::Shockwave:
		return TEXT("SHOCKWAVE");
	default:
		return TEXT("READY");
	}
}

void ACWSBossEnemy::HandleBossHealthChanged(
	AActor* DamagedActor,
	const float CurrentHealth,
	const float MaxHealth,
	AActor* ChangeInstigator)
{
	UpdateBossPhase(MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f);
}

void ACWSBossEnemy::UpdateBossPhase(const float HealthPercent)
{
	ECWSBossPhase NewPhase = ECWSBossPhase::PhaseOne;
	if (HealthPercent <= 0.33f)
	{
		NewPhase = ECWSBossPhase::FinalPhase;
	}
	else if (HealthPercent <= 0.66f)
	{
		NewPhase = ECWSBossPhase::PhaseTwo;
	}

	if (BossPhase == NewPhase)
	{
		return;
	}

	BossPhase = NewPhase;
	GetCharacterMovement()->MaxWalkSpeed = BossPhase == ECWSBossPhase::PhaseTwo ? 320.0f : 380.0f;
	NextPatternTime = 0.0f;
	OnBossPhaseChanged.Broadcast(BossPhase);
	UE_LOG(LogCWSBoss, Display, TEXT("Boss entered %s."), *GetBossPhaseLabel());
}

bool ACWSBossEnemy::ExecuteGroundSlam(AActor* TargetActor)
{
	if (FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(GroundSlamRadius))
	{
		return false;
	}
	UGameplayStatics::ApplyDamage(TargetActor, GroundSlamDamage, GetController(), this, UDamageType::StaticClass());
	return true;
}

bool ACWSBossEnemy::ExecuteShockwave(AActor* TargetActor)
{
	const FVector Offset = TargetActor->GetActorLocation() - GetActorLocation();
	if (Offset.SizeSquared() > FMath::Square(ShockwaveRadius))
	{
		return false;
	}

	UGameplayStatics::ApplyDamage(TargetActor, ShockwaveDamage, GetController(), this, UDamageType::StaticClass());
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		const FVector LaunchDirection = Offset.GetSafeNormal2D();
		TargetCharacter->LaunchCharacter(LaunchDirection * ShockwaveStrength + FVector(0.0f, 0.0f, 350.0f), true, true);
	}
	return true;
}

void ACWSBossEnemy::RecordPattern(const ECWSBossPattern Pattern)
{
	LastPattern = Pattern;
	++PatternExecutionCount;
	OnBossPatternExecuted.Broadcast(Pattern);
	UE_LOG(LogCWSBoss, Display, TEXT("Boss executed %s in %s."), *GetLastPatternLabel(), *GetBossPhaseLabel());
}
