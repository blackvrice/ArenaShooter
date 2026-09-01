#pragma once

#include "CoreMinimal.h"
#include "Enemy/CWSEnemyBase.h"
#include "CWSTankEnemy.generated.h"

/** 높은 체력과 공격력을 대가로 이동과 공격 주기가 느린 적 변형입니다. */
UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSTankEnemy : public ACWSEnemyBase
{
	GENERATED_BODY()

public:
	ACWSTankEnemy();
};
