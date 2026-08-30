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
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void Fire(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);
	void RestartLevel(const FInputActionValue& Value);
	bool CanUseGameplayInput() const;
	void UpdateRifleAnimation();
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

	FTransform WeaponRestRelativeTransform = FTransform::Identity;
	float WeaponRecoilElapsed = 0.0f;
	float WeaponRecoilDuration = 0.14f;
	bool bWeaponRecoilActive = false;
	float RifleActionEndTime = 0.0f;
};
