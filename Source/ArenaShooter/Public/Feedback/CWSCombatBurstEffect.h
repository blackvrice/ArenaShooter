#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWSCombatBurstEffect.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * 외부 Niagara 캐시에 의존하지 않는 짧은 피격/사망 버스트입니다.
 * 런타임 생성 메시와 라이트만 사용해 Editor와 Shipping에서 동일한 경로로 동작합니다.
 */
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

	// Tick에서 0~1 수명 비율을 계산해 메시 확장과 라이트 감쇠를 함께 구동합니다.
	float ElapsedTime = 0.0f;
	float EffectDuration = 0.32f;
	float TargetScale = 0.32f;
	float InitialLightIntensity = 4500.0f;
};
