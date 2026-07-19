#include "Enemy/CWSEnemyAIController.h"

#include "Components/CWSHealthComponent.h"
#include "Enemy/CWSEnemyBase.h"
#include "Kismet/GameplayStatics.h"

ACWSEnemyAIController::ACWSEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
}

void ACWSEnemyAIController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ACWSEnemyBase* Enemy = Cast<ACWSEnemyBase>(GetPawn());
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Enemy || !PlayerPawn || !Enemy->GetHealthComponent()->IsAlive())
	{
		StopMovement();
		return;
	}

	if (const UCWSHealthComponent* PlayerHealth = PlayerPawn->FindComponentByClass<UCWSHealthComponent>();
		PlayerHealth && !PlayerHealth->IsAlive())
	{
		StopMovement();
		return;
	}

	const float DistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistanceSquared <= FMath::Square(Enemy->GetAttackRange()))
	{
		StopMovement();
		Enemy->TryAttack(PlayerPawn);
		return;
	}

	MoveToActor(PlayerPawn, Enemy->GetAttackRange() * 0.75f, true, true, true, nullptr, true);
}
