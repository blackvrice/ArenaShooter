#include "Enemy/CWSTankEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ACWSTankEnemy::ACWSTankEnemy()
{
	EnemyType = ECWSEnemyType::Tank;
	HealthComponent->SetMaxHealth(180.0f);
	AttackDamage = 18.0f;
	AttackRange = 180.0f;
	AttackInterval = 1.4f;
	GetCharacterMovement()->MaxWalkSpeed = 230.0f;
	GetCapsuleComponent()->InitCapsuleSize(52.0f, 116.0f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
		TEXT("/Game/CWSResources/Enemies/Tank/SK_Tank.SK_Tank"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleAsset(
		TEXT("/Game/CWSResources/Enemies/Tank/A_Tank_Idle.A_Tank_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> MoveAsset(
		TEXT("/Game/CWSResources/Enemies/Tank/A_Tank_Move.A_Tank_Move"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AttackAsset(
		TEXT("/Game/CWSResources/Enemies/Tank/A_Tank_Attack.A_Tank_Attack"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitAsset(
		TEXT("/Game/CWSResources/Enemies/Tank/A_Tank_Hit.A_Tank_Hit"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DeathAsset(
		TEXT("/Game/CWSResources/Enemies/Tank/A_Tank_Death.A_Tank_Death"));
	ConfigureEnemyVisualProfile(
		MeshAsset.Object,
		IdleAsset.Object,
		MoveAsset.Object,
		AttackAsset.Object,
		HitAsset.Object,
		DeathAsset.Object,
		FVector(0.0f, 0.0f, -116.0f),
		FVector(1.0f));
}
