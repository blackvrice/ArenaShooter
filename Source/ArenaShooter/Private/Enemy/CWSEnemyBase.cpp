#include "Enemy/CWSEnemyBase.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Enemy/CWSEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
}

void ACWSEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &ACWSEnemyBase::HandleDeath);
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
	UGameplayStatics::ApplyDamage(TargetActor, AttackDamage, GetController(), this, UDamageType::StaticClass());
	return true;
}

float ACWSEnemyBase::GetMoveSpeed() const
{
	return GetCharacterMovement()->MaxWalkSpeed;
}

void ACWSEnemyBase::HandleDeath(AActor* DeadActor)
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromControllerPendingDestroy();
	SetLifeSpan(0.25f);
}
