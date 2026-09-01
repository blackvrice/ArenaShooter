#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CWSEnemyAIController.generated.h"

/**
 * 적과 플레이어가 살아 있는 동안 NavMesh로 접근하고, 공격 거리에서는 이동을 멈춥니다.
 * 공격 가능 시각과 실제 피해 규칙은 Pawn의 TryAttack에 위임합니다.
 */
UCLASS()
class ARENASHOOTER_API ACWSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACWSEnemyAIController();

	virtual void Tick(float DeltaSeconds) override;
};
