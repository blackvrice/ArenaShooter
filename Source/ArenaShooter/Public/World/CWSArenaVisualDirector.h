#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWSArenaVisualDirector.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;

UCLASS()
class ARENASHOOTER_API ACWSArenaVisualDirector : public AActor
{
	GENERATED_BODY()

public:
	ACWSArenaVisualDirector();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Arena|Presentation")
	int32 GetCenterRingSegmentCount() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Presentation")
	int32 GetCoverCount() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Presentation")
	int32 GetGateBeaconCount() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Presentation")
	bool IsPresentationReady() const { return bPresentationReady; }

	UFUNCTION(BlueprintPure, Category = "Arena|Presentation")
	bool HasBlockingCover() const;

private:
	void ConfigureMeshComponent(UInstancedStaticMeshComponent* Component, bool bBlocksMovement);
	bool ApplyColor(UInstancedStaticMeshComponent* Component, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> CenterRing;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> CoverBlocks;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> NorthGate;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> SouthGate;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> EastGate;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> WestGate;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UPointLightComponent> NorthGateLight;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UPointLightComponent> SouthGateLight;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UPointLightComponent> EastGateLight;

	UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
	TObjectPtr<UPointLightComponent> WestGateLight;

	bool bPresentationReady = false;
};
