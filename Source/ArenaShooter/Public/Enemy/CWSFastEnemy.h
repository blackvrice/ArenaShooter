#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyBase.h"
#include "CWSFastEnemy.generated.h"

/** 낮은 체력과 빠른 이동/공격 주기로 측면 압박을 만드는 적 변형입니다. */
UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSFastEnemy : public ACWSEnemyBase
{
	GENERATED_BODY()

public:
	ACWSFastEnemy();
};
