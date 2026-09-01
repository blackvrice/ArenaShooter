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

	// 제곱 거리를 사용해 매 Tick의 불필요한 sqrt를 피한다.
	const float DistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistanceSquared <= FMath::Square(Enemy->GetAttackRange()))
	{
		StopMovement();
		Enemy->TryAttack(PlayerPawn);
		return;
	}

	// AcceptanceRadius를 공격 거리보다 작게 둬 NavMesh 경계 오차가 있어도 TryAttack 범위에 진입한다.
	MoveToActor(PlayerPawn, Enemy->GetAttackRange() * 0.75f, true, true, true, nullptr, true);
}
