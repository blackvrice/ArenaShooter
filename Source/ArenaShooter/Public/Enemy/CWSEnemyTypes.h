#pragma once

#include "CoreMinimal.h"
#include "CWSEnemyTypes.generated.h"

/** Wave 정의, 스폰 클래스 선택, HUD/검증에서 공유하는 적 아키타입 식별자입니다. */
UENUM(BlueprintType)
enum class ECWSEnemyType : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Fast UMETA(DisplayName = "Fast"),
	Tank UMETA(DisplayName = "Tank"),
	Boss UMETA(DisplayName = "Boss")
};
