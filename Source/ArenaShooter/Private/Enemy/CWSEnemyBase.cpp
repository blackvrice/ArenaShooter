#include "Enemy/CWSEnemyBase.h"

#include "Audio/CWSCombatSound.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Enemy/CWSEnemyAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACWSEnemyBase::ACWSEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthComponent = CreateDefaultSubobject<UCWSHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(60.0f);

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	bUseControllerRotationYaw = false;

	AIControllerClass = ACWSEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (EnemyMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(EnemyMeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> EnemyAnimClass(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (EnemyAnimClass.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(EnemyAnimClass.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitReactionAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01"));
	HitReactionAnimation = HitReactionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AttackAnimationAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	AttackAnimation = AttackAnimationAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DeathAnimationAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	DeathAnimation = DeathAnimationAsset.Object;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DeathEffectAsset(
		TEXT("/Game/Variant_Combat/VFX/NS_Damage.NS_Damage"));
	DeathEffect = DeathEffectAsset.Object;
}

void ACWSEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	LastObservedHealth = HealthComponent->GetCurrentHealth();
	HealthComponent->OnHealthChanged.AddDynamic(this, &ACWSEnemyBase::HandleHealthChanged);
	HealthComponent->OnDeath.AddDynamic(this, &ACWSEnemyBase::HandleDeath);
}

void ACWSEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(HitReactionTimerHandle);
	Super::EndPlay(EndPlayReason);
}

bool ACWSEnemyBase::TryAttack(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetActor) || !HealthComponent->IsAlive())
	{
		return false;
	}

	const float DistanceSquared = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceSquared > FMath::Square(AttackRange) || World->GetTimeSeconds() < NextAllowedAttackTime)
	{
		return false;
	}

	NextAllowedAttackTime = World->GetTimeSeconds() + AttackInterval;
	PlayAttackAnimation();
	if (PlayCWSCombatSound(this, GetActorLocation(), ECWSCombatSoundType::EnemyAttack, 0.8f))
	{
		++AttackSoundPlayCount;
	}
	UGameplayStatics::ApplyDamage(TargetActor, AttackDamage, GetController(), this, UDamageType::StaticClass());
	return true;
}

bool ACWSEnemyBase::PlayAttackAnimation()
{
	if (!AttackAnimation)
	{
		return false;
	}
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (AnimInstance->PlaySlotAnimationAsDynamicMontage(
			AttackAnimation,
			TEXT("DefaultSlot"),
			0.04f,
			0.12f,
			1.0f,
			1,
			0.0f,
			0.0f))
		{
			++AttackAnimationCount;
			return true;
		}
	}
	return false;
}

bool ACWSEnemyBase::StageAttackPoseForCapture(const float NormalizedTime)
{
	if (!AttackAnimation)
	{
		return false;
	}
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(AttackAnimation, false);
	if (UAnimSingleNodeInstance* AnimationInstance = GetMesh()->GetSingleNodeInstance())
	{
		AnimationInstance->SetPosition(
			AttackAnimation->GetPlayLength() * FMath::Clamp(NormalizedTime, 0.0f, 1.0f),
			false);
		return true;
	}
	return false;
}

float ACWSEnemyBase::GetMoveSpeed() const
{
	return GetCharacterMovement()->MaxWalkSpeed;
}

void ACWSEnemyBase::HandleHealthChanged(
	AActor* DamagedActor,
	const float CurrentHealth,
	const float MaxHealth,
	AActor* ChangeInstigator)
{
	const bool bTookNonLethalDamage = CurrentHealth > 0.0f && CurrentHealth < LastObservedHealth;
	LastObservedHealth = CurrentHealth;
	if (!bTookNonLethalDamage || !HitReactionAnimation)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		UAnimMontage* Montage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
			HitReactionAnimation,
			TEXT("DefaultSlot"),
			0.04f,
			0.12f,
			1.0f,
			1,
			0.0f,
			0.0f);
		if (Montage)
		{
			++HitReactionCount;
			bHitReactionActive = true;
			GetWorldTimerManager().SetTimer(
				HitReactionTimerHandle,
				this,
				&ACWSEnemyBase::FinishHitReaction,
				FMath::Min(Montage->GetPlayLength(), 0.55f),
				false);
		}
	}
}

void ACWSEnemyBase::FinishHitReaction()
{
	bHitReactionActive = false;
}

void ACWSEnemyBase::PlayFeedbackAnimation(UAnimSequenceBase* Animation)
{
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, false);
	bDeathAnimationPlayed = true;
}

bool ACWSEnemyBase::SpawnDeathEffect()
{
	if (!DeathEffect || !GetWorld())
	{
		return false;
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		DeathEffect,
		GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
		FRotator::ZeroRotator,
		FVector(3.0f));
	return true;
}

void ACWSEnemyBase::HandleDeath(AActor* DeadActor)
{
	GetWorldTimerManager().ClearTimer(HitReactionTimerHandle);
	bHitReactionActive = false;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromControllerPendingDestroy();
	if (DeathAnimation)
	{
		PlayFeedbackAnimation(DeathAnimation);
	}
	if (SpawnDeathEffect())
	{
		++DeathEffectSpawnCount;
	}
	SetLifeSpan(2.0f);
}
