#include "Enemy/CWSFastEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ACWSFastEnemy::ACWSFastEnemy()
{
	EnemyType = ECWSEnemyType::Fast;
	HealthComponent->SetMaxHealth(35.0f);
	AttackDamage = 8.0f;
	AttackRange = 150.0f;
	AttackInterval = 0.65f;
	GetCharacterMovement()->MaxWalkSpeed = 520.0f;
	GetCapsuleComponent()->InitCapsuleSize(36.0f, 82.0f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
		TEXT("/Game/CWSResources/Enemies/Fast/SK_FastMinion.SK_FastMinion"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleAsset(
		TEXT("/Game/CWSResources/Enemies/Fast/A_Fast_Idle.A_Fast_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> MoveAsset(
		TEXT("/Game/CWSResources/Enemies/Fast/A_Fast_Move.A_Fast_Move"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AttackAsset(
		TEXT("/Game/CWSResources/Enemies/Fast/A_Fast_Attack.A_Fast_Attack"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitAsset(
		TEXT("/Game/CWSResources/Enemies/Fast/A_Fast_Hit.A_Fast_Hit"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DeathAsset(
		TEXT("/Game/CWSResources/Enemies/Fast/A_Fast_Death.A_Fast_Death"));
	ConfigureEnemyVisualProfile(
		MeshAsset.Object,
		IdleAsset.Object,
		MoveAsset.Object,
		AttackAsset.Object,
		HitAsset.Object,
		DeathAsset.Object,
		FVector(0.0f, 0.0f, -82.0f),
		FVector(0.9f));
}
