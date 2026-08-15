#pragma once

#include "CoreMinimal.h"
#include "CWSEnemyTypes.generated.h"

UENUM(BlueprintType)
enum class ECWSEnemyType : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Fast UMETA(DisplayName = "Fast"),
	Tank UMETA(DisplayName = "Tank"),
	Boss UMETA(DisplayName = "Boss")
};
