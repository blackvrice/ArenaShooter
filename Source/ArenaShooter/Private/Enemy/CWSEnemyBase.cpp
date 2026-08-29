#include "Enemy/CWSEnemyBase.h"

#include "Audio/CWSCombatSound.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemy/CWSEnemyAIController.h"
#include "Feedback/CWSCombatBurstEffect.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSEnemyResources, Log, All);

ACWSEnemyBase::ACWSEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UCWSHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(60.0f);

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	bUseControllerRotationYaw = false;

	ArchetypeMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArchetypeMarker"));
	ArchetypeMarker->SetupAttachment(GetCapsuleComponent());
	ArchetypeMarker->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	ArchetypeMarker->SetRelativeScale3D(FVector(0.18f));
	ArchetypeMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArchetypeMarker->SetCastShadow(false);

	ArchetypeBand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArchetypeBand"));
	ArchetypeBand->SetupAttachment(GetCapsuleComponent());
	ArchetypeBand->SetRelativeLocation(FVector(0.0f, 0.0f, -91.0f));
	ArchetypeBand->SetRelativeScale3D(FVector(0.52f, 0.52f, 0.025f));
	ArchetypeBand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArchetypeBand->SetCastShadow(false);

	ArchetypeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ArchetypeLight"));
	ArchetypeLight->SetupAttachment(GetCapsuleComponent());
	ArchetypeLight->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	ArchetypeLight->SetIntensity(850.0f);
	ArchetypeLight->SetAttenuationRadius(260.0f);
	ArchetypeLight->SetSourceRadius(12.0f);
	ArchetypeLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		ArchetypeMarker->SetStaticMesh(SphereMeshAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshAsset.Succeeded())
	{
		ArchetypeBand->SetStaticMesh(CylinderMeshAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicShapeMaterialAsset.Succeeded())
	{
		ArchetypeMarker->SetMaterial(0, BasicShapeMaterialAsset.Object);
		ArchetypeBand->SetMaterial(0, BasicShapeMaterialAsset.Object);
	}

	AIControllerClass = ACWSEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMeshAsset(
		TEXT("/Game/CWSResources/Enemies/Normal/SK_NormalMinion.SK_NormalMinion"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleAnimationAsset(
		TEXT("/Game/CWSResources/Enemies/Normal/A_Normal_Idle.A_Normal_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> MoveAnimationAsset(
		TEXT("/Game/CWSResources/Enemies/Normal/A_Normal_Move.A_Normal_Move"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AttackAnimationAsset(
		TEXT("/Game/CWSResources/Enemies/Normal/A_Normal_Attack.A_Normal_Attack"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitReactionAsset(
		TEXT("/Game/CWSResources/Enemies/Normal/A_Normal_Hit.A_Normal_Hit"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DeathAnimationAsset(
		TEXT("/Game/CWSResources/Enemies/Normal/A_Normal_Death.A_Normal_Death"));
	ConfigureEnemyVisualProfile(
		EnemyMeshAsset.Object,
		IdleAnimationAsset.Object,
		MoveAnimationAsset.Object,
		AttackAnimationAsset.Object,
		HitReactionAsset.Object,
		DeathAnimationAsset.Object,
		FVector(0.0f, 0.0f, -96.0f),
		FVector(1.0f));
}

void ACWSEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor ArchetypeColor = GetArchetypeColor();
	UMaterialInstanceDynamic* MarkerMaterial = ArchetypeMarker->CreateDynamicMaterialInstance(0);
	UMaterialInstanceDynamic* BandMaterial = ArchetypeBand->CreateDynamicMaterialInstance(0);
	if (MarkerMaterial)
	{
		MarkerMaterial->SetVectorParameterValue(TEXT("Color"), ArchetypeColor);
	}
	if (BandMaterial)
	{
		BandMaterial->SetVectorParameterValue(TEXT("Color"), ArchetypeColor);
	}
	ArchetypeLight->SetLightColor(ArchetypeColor);
	bArchetypePresentationReady = MarkerMaterial && BandMaterial && GetMesh()->GetSkeletalMeshAsset() &&
		IdleAnimation && MoveAnimation && AttackAnimation && HitReactionAnimation && DeathAnimation;

	const float MarkerScale = EnemyType == ECWSEnemyType::Fast
		? 0.14f
		: EnemyType == ECWSEnemyType::Tank ? 0.24f : EnemyType == ECWSEnemyType::Boss ? 0.32f : 0.18f;
	const float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	ArchetypeMarker->SetRelativeLocation(FVector(0.0f, 0.0f, CapsuleHalfHeight + 45.0f));
	ArchetypeBand->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleHalfHeight + 4.0f));
	ArchetypeMarker->SetRelativeScale3D(FVector(MarkerScale));
	ArchetypeBand->SetRelativeScale3D(FVector(MarkerScale * 3.0f, MarkerScale * 3.0f, 0.025f));
	LastObservedHealth = HealthComponent->GetCurrentHealth();
	HealthComponent->OnHealthChanged.AddDynamic(this, &ACWSEnemyBase::HandleHealthChanged);
	HealthComponent->OnDeath.AddDynamic(this, &ACWSEnemyBase::HandleDeath);
	PlayLoopingAnimation(IdleAnimation);

	UE_LOG(
		LogCWSEnemyResources,
		Display,
		TEXT("CWS_ENEMY_RESOURCE: Type=%s Mesh=%s Idle=%s Move=%s Attack=%s Hit=%s Death=%s"),
		*GetArchetypeLabel(),
		*GetVisualMeshPath(),
		*GetNameSafe(IdleAnimation),
		*GetNameSafe(MoveAnimation),
		*GetNameSafe(AttackAnimation),
		*GetNameSafe(HitReactionAnimation),
		*GetNameSafe(DeathAnimation));
}

void ACWSEnemyBase::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLocomotionAnimation();
}

FLinearColor ACWSEnemyBase::GetArchetypeColor() const
{
	switch (EnemyType)
	{
	case ECWSEnemyType::Fast:
		return FLinearColor(1.0f, 0.24f, 0.03f);
	case ECWSEnemyType::Tank:
		return FLinearColor(0.05f, 0.35f, 1.0f);
	case ECWSEnemyType::Boss:
		return FLinearColor(0.75f, 0.05f, 1.0f);
	case ECWSEnemyType::Normal:
	default:
		return FLinearColor(0.05f, 0.95f, 0.35f);
	}
}

FString ACWSEnemyBase::GetArchetypeLabel() const
{
	switch (EnemyType)
	{
	case ECWSEnemyType::Fast:
		return TEXT("FAST");
	case ECWSEnemyType::Tank:
		return TEXT("TANK");
	case ECWSEnemyType::Boss:
		return TEXT("BOSS");
	case ECWSEnemyType::Normal:
	default:
		return TEXT("NORMAL");
	}
}

FString ACWSEnemyBase::GetVisualMeshPath() const
{
	return GetMesh() && GetMesh()->GetSkeletalMeshAsset()
		? GetMesh()->GetSkeletalMeshAsset()->GetPathName()
		: FString();
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
	if (PlayActionAnimation(AttackAnimation))
	{
		++AttackAnimationCount;
		return true;
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

	if (PlayActionAnimation(HitReactionAnimation))
	{
		++HitReactionCount;
		bHitReactionActive = true;
		GetWorldTimerManager().SetTimer(
			HitReactionTimerHandle,
			this,
			&ACWSEnemyBase::FinishHitReaction,
			FMath::Min(HitReactionAnimation->GetPlayLength(), 0.55f),
			false);
	}
}

void ACWSEnemyBase::FinishHitReaction()
{
	bHitReactionActive = false;
}

void ACWSEnemyBase::PlayFeedbackAnimation(UAnimSequenceBase* Animation)
{
	bDeathAnimationPlayed = PlayActionAnimation(Animation);
}

void ACWSEnemyBase::ConfigureEnemyVisualProfile(
	USkeletalMesh* MeshAsset,
	UAnimSequenceBase* IdleAsset,
	UAnimSequenceBase* MoveAsset,
	UAnimSequenceBase* AttackAsset,
	UAnimSequenceBase* HitReactionAsset,
	UAnimSequenceBase* DeathAsset,
	const FVector& RelativeLocation,
	const FVector& RelativeScale)
{
	GetMesh()->SetSkeletalMesh(MeshAsset);
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->SetRelativeLocation(RelativeLocation);
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeScale3D(RelativeScale);
	IdleAnimation = IdleAsset;
	MoveAnimation = MoveAsset;
	AttackAnimation = AttackAsset;
	HitReactionAnimation = HitReactionAsset;
	DeathAnimation = DeathAsset;
}

void ACWSEnemyBase::PlayLoopingAnimation(UAnimSequenceBase* Animation)
{
	if (!Animation || CurrentLoopingAnimation == Animation)
	{
		return;
	}
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, true);
	CurrentLoopingAnimation = Animation;
}

bool ACWSEnemyBase::PlayActionAnimation(UAnimSequenceBase* Animation)
{
	if (!Animation || !GetWorld())
	{
		return false;
	}
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, false);
	CurrentLoopingAnimation = nullptr;
	ActionAnimationEndTime = GetWorld()->GetTimeSeconds() + Animation->GetPlayLength();
	return true;
}

void ACWSEnemyBase::UpdateLocomotionAnimation()
{
	if (!HealthComponent || !HealthComponent->IsAlive() || !GetWorld() || bHitReactionActive ||
		GetWorld()->GetTimeSeconds() < ActionAnimationEndTime)
	{
		return;
	}
	PlayLoopingAnimation(GetVelocity().SizeSquared2D() > FMath::Square(10.0f) ? MoveAnimation : IdleAnimation);
}

bool ACWSEnemyBase::SpawnDeathEffect()
{
	return ACWSCombatBurstEffect::SpawnBurst(
		GetWorld(),
		GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
		GetArchetypeColor(),
		1.35f,
		0.55f);
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
