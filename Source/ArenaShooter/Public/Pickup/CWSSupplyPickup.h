#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWSSupplyPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class ECWSSupplyType : uint8
{
	Health UMETA(DisplayName = "Health"),
	Ammo UMETA(DisplayName = "Ammo")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCWSSupplyCollectedEvent, AActor*, Collector, ECWSSupplyType, SupplyType);

/**
 * 라운드 클리어 후 생성되는 체력/탄약 보급 Actor입니다.
 * 실제로 자원이 증가한 경우에만 수집 처리하므로, 가득 찬 자원 때문에 보급이 사라지지 않습니다.
 */
UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSSupplyPickup : public AActor
{
	GENERATED_BODY()

public:
	ACWSSupplyPickup();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Supply")
	void ConfigureSupply(ECWSSupplyType NewSupplyType);

	UFUNCTION(BlueprintCallable, Category = "Supply")
	bool TryCollect(AActor* Collector);

	UFUNCTION(BlueprintPure, Category = "Supply")
	ECWSSupplyType GetSupplyType() const { return SupplyType; }

	UFUNCTION(BlueprintPure, Category = "Supply")
	float GetHealthAmount() const { return HealthAmount; }

	UFUNCTION(BlueprintPure, Category = "Supply")
	int32 GetAmmoAmount() const { return AmmoAmount; }

	UPROPERTY(BlueprintAssignable, Category = "Supply|Events")
	FCWSSupplyCollectedEvent OnSupplyCollected;

private:
	UFUNCTION()
	void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void UpdatePresentation();

	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<USphereComponent> CollectionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<UStaticMeshComponent> SupplyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<UTextRenderComponent> SupplyLabel;

	UPROPERTY(EditAnywhere, Category = "Supply")
	ECWSSupplyType SupplyType = ECWSSupplyType::Health;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "1.0"))
	float HealthAmount = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "1"))
	int32 AmmoAmount = 90;

	UPROPERTY(EditAnywhere, Category = "Supply|Visual", meta = (ClampMin = "0.0"))
	float BobHeight = 18.0f;

	UPROPERTY(EditAnywhere, Category = "Supply|Visual", meta = (ClampMin = "0.0"))
	float BobSpeed = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Supply|Visual", meta = (ClampMin = "0.0"))
	float RotationSpeed = 60.0f;

	FVector BaseLocation = FVector::ZeroVector;
	float RunningTime = 0.0f;
	// Overlap이 같은 프레임에 중복 호출되어 보상이 두 번 적용되는 것을 막습니다.
	bool bCollected = false;
};
