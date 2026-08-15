#include "Enemy/CWSTankEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ACWSTankEnemy::ACWSTankEnemy()
{
	EnemyType = ECWSEnemyType::Tank;
	HealthComponent->SetMaxHealth(180.0f);
	AttackDamage = 18.0f;
	AttackRange = 180.0f;
	AttackInterval = 1.4f;
	GetCharacterMovement()->MaxWalkSpeed = 230.0f;
	GetCapsuleComponent()->InitCapsuleSize(52.0f, 116.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -116.0f));
	GetMesh()->SetRelativeScale3D(FVector(1.25f));
}
