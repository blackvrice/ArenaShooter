#include "Enemy/CWSFastEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ACWSFastEnemy::ACWSFastEnemy()
{
	EnemyType = ECWSEnemyType::Fast;
	HealthComponent->SetMaxHealth(35.0f);
	AttackDamage = 8.0f;
	AttackRange = 150.0f;
	AttackInterval = 0.65f;
	GetCharacterMovement()->MaxWalkSpeed = 520.0f;
	GetCapsuleComponent()->InitCapsuleSize(36.0f, 82.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -82.0f));
	GetMesh()->SetRelativeScale3D(FVector(0.85f));
}
