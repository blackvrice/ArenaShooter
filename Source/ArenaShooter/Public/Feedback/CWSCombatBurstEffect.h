#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWSCombatBurstEffect.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(NotBlueprintable, Transient)
class ARENASHOOTER_API ACWSCombatBurstEffect : public AActor
{
	GENERATED_BODY()

public:
	ACWSCombatBurstEffect();

	virtual void Tick(float DeltaSeconds) override;

	static bool SpawnBurst(
		UWorld* World,
		const FVector& Location,
		const FLinearColor& Color,
		float MaximumScale,
		float Duration = 0.32f);

private:
	void Configure(const FLinearColor& Color, float MaximumScale, float Duration);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BurstMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> BurstLight;

	float ElapsedTime = 0.0f;
	float EffectDuration = 0.32f;
	float TargetScale = 0.32f;
	float InitialLightIntensity = 4500.0f;
};
