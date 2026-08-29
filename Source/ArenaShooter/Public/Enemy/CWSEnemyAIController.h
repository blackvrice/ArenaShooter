#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CWSEnemyAIController.generated.h"

UCLASS()
class ARENASHOOTER_API ACWSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACWSEnemyAIController();

	virtual void Tick(float DeltaSeconds) override;
};
