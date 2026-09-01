#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "CWSPlayerCharacter.generated.h"

class UCameraComponent;
class UCWSHealthComponent;
class UCWSHitscanWeaponComponent;
class UAnimSequence;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * 플레이어 입력과 3인칭 표현을 무기/체력 컴포넌트에 연결하는 캐릭터입니다.
 *
 * 사격 판정과 탄약 규칙은 UCWSHitscanWeaponComponent가 소유하고, 이 클래스는
 * 입력 허용 여부, 이동, 재장전 동작, 라이플 애니메이션과 반동 표현을 담당합니다.
 */
UCLASS(Blueprintable)
class ARENASHOOTER_API ACWSPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACWSPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Player")
	UCWSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UCWSHitscanWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

private:
	// Enhanced Input 콜백: Title 대기/게임 종료 상태에서는 CanUseGameplayInput으로 차단됩니다.
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void Fire(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);
	void RestartLevel(const FInputActionValue& Value);
	bool CanUseGameplayInput() const;
	/** 이동 상태에 맞는 반복 애니메이션을 고르되 일회성 액션 중에는 교체하지 않습니다. */
	void UpdateRifleAnimation();
	/** Additive 발사 클립 대신 부착된 총기 Transform만 짧게 움직입니다. */
	void UpdateWeaponRecoil(float DeltaSeconds);
	void PlayRifleAnimation(UAnimSequence* Animation, bool bLooping, float PlayRate = 1.0f);
	void PlayWeaponFireRecoil();
	void PlayRifleAction(UAnimSequence* Animation, float Duration);
	UAnimSequence* SelectRifleMovementAnimation() const;

	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Player")
	TObjectPtr<UCWSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Player")
	TObjectPtr<UCWSHitscanWeaponComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY()
	TObjectPtr<UAnimSequence> RifleIdleAnimation;

	UPROPERTY()
	TArray<TObjectPtr<UAnimSequence>> RifleJogAnimations;

	UPROPERTY()
	TObjectPtr<UAnimSequence> RifleJumpAnimation;

	UPROPERTY()
	TObjectPtr<UAnimSequence> RifleFallAnimation;

	UPROPERTY()
	TObjectPtr<UAnimSequence> RifleReloadAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentRifleAnimation;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> CombatMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY()
	TObjectPtr<UInputAction> RestartAction;

	// 반동은 매번 이 기준 Transform에서 계산해 프레임별 오차가 누적되지 않습니다.
	FTransform WeaponRestRelativeTransform = FTransform::Identity;
	float WeaponRecoilElapsed = 0.0f;
	float WeaponRecoilDuration = 0.14f;
	bool bWeaponRecoilActive = false;
	// Reload 같은 일회성 동작이 locomotion 갱신에 즉시 덮이지 않도록 잠그는 시각입니다.
	float RifleActionEndTime = 0.0f;
};
