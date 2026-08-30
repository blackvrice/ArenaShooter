#include "Player/CWSPlayerCharacter.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UAnimSequence* LoadRifleAnimation(const TCHAR* AssetPath)
	{
		const ConstructorHelpers::FObjectFinder<UAnimSequence> AnimationAsset(AssetPath);
		return AnimationAsset.Object;
	}
}

ACWSPlayerCharacter::ACWSPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 70.0f, 65.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UCWSHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(100.0f);
	WeaponComponent = CreateDefaultSubobject<UCWSHitscanWeaponComponent>(TEXT("WeaponComponent"));
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("ik_hand_gun"));
	WeaponMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RifleMeshAsset(
		TEXT("/Game/Weapons/Rifle/Mesh/SM_Rifle.SM_Rifle"));
	if (RifleMeshAsset.Succeeded())
	{
		WeaponMesh->SetStaticMesh(RifleMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PlayerMeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (PlayerMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(PlayerMeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	RifleIdleAnimation = LoadRifleAnimation(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
	RifleJogAnimations = {
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd.MF_Rifle_Jog_Fwd")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd_Right.MF_Rifle_Jog_Fwd_Right")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Right.MF_Rifle_Jog_Right")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Bwd_Right.MF_Rifle_Jog_Bwd_Right")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Bwd.MF_Rifle_Jog_Bwd")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Bwd_Left.MF_Rifle_Jog_Bwd_Left")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Left.MF_Rifle_Jog_Left")),
		LoadRifleAnimation(TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd_Left.MF_Rifle_Jog_Fwd_Left")),
	};
	RifleJumpAnimation = LoadRifleAnimation(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jump/MM_Rifle_Jump_Start_Loop.MM_Rifle_Jump_Start_Loop"));
	RifleFallAnimation = LoadRifleAnimation(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jump/MM_Rifle_Jump_Fall_Loop.MM_Rifle_Jump_Fall_Loop"));
	RifleReloadAnimation = LoadRifleAnimation(
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload"));

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContextAsset(
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	DefaultMappingContext = DefaultContextAsset.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(
		TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	MoveAction = MoveActionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionAsset(
		TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	LookAction = LookActionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookActionAsset(
		TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	MouseLookAction = MouseLookActionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionAsset(
		TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	JumpAction = JumpActionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionAsset(
		TEXT("/Game/Variant_Combat/Input/Actions/IA_ComboAttack.IA_ComboAttack"));
	FireAction = FireActionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadActionAsset(
		TEXT("/Game/Variant_Combat/Input/Actions/IA_ChargedAttack.IA_ChargedAttack"));
	ReloadAction = ReloadActionAsset.Object;
	RestartAction = CreateDefaultSubobject<UInputAction>(TEXT("RestartAction"));
	RestartAction->ValueType = EInputActionValueType::Boolean;
}

void ACWSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &ACWSPlayerCharacter::HandleDeath);
	WeaponRestRelativeTransform = WeaponMesh->GetRelativeTransform();
	PlayRifleAnimation(RifleIdleAnimation, true);
	CombatMappingContext = NewObject<UInputMappingContext>(this, TEXT("CWSCombatMappingContext"));
	if (FireAction)
	{
		CombatMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	}
	if (ReloadAction)
	{
		CombatMappingContext->MapKey(ReloadAction, EKeys::RightMouseButton);
	}
	if (MouseLookAction)
	{
		CombatMappingContext->MapKey(MouseLookAction, EKeys::Mouse2D);
	}
	CombatMappingContext->MapKey(RestartAction, EKeys::Enter);

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			if (CombatMappingContext)
			{
				InputSubsystem->AddMappingContext(CombatMappingContext, 1);
			}
		}
	}
}

void ACWSPlayerCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateRifleAnimation();
	UpdateWeaponRecoil(DeltaSeconds);
}

void ACWSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACWSPlayerCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACWSPlayerCharacter::Look);
	}
	if (MouseLookAction)
	{
		EnhancedInput->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACWSPlayerCharacter::Look);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACWSPlayerCharacter::StartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACWSPlayerCharacter::StopJump);
	}
	if (FireAction)
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Triggered, this, &ACWSPlayerCharacter::Fire);
	}
	if (ReloadAction)
	{
		EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &ACWSPlayerCharacter::Reload);
	}
	EnhancedInput->BindAction(RestartAction, ETriggerEvent::Started, this, &ACWSPlayerCharacter::RestartLevel);
}

void ACWSPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller || !CanUseGameplayInput())
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void ACWSPlayerCharacter::Look(const FInputActionValue& Value)
{
	if (!CanUseGameplayInput())
	{
		return;
	}
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void ACWSPlayerCharacter::StartJump(const FInputActionValue& Value)
{
	if (CanUseGameplayInput())
	{
		Jump();
	}
}

void ACWSPlayerCharacter::StopJump(const FInputActionValue& Value)
{
	StopJumping();
}

void ACWSPlayerCharacter::Fire(const FInputActionValue& Value)
{
	if (CanUseGameplayInput() && HealthComponent->IsAlive())
	{
		if (WeaponComponent->TryFire())
		{
			PlayWeaponFireRecoil();
		}
		else if (WeaponComponent->IsReloading() && CurrentRifleAnimation != RifleReloadAnimation)
		{
			PlayRifleAction(RifleReloadAnimation, WeaponComponent->GetReloadDuration());
		}
	}
}

void ACWSPlayerCharacter::Reload(const FInputActionValue& Value)
{
	if (CanUseGameplayInput() && HealthComponent->IsAlive())
	{
		if (WeaponComponent->Reload())
		{
			PlayRifleAction(RifleReloadAnimation, WeaponComponent->GetReloadDuration());
		}
	}
}

void ACWSPlayerCharacter::UpdateRifleAnimation()
{
	if (!HealthComponent || !HealthComponent->IsAlive() || !GetWorld() || GetWorld()->GetTimeSeconds() < RifleActionEndTime)
	{
		return;
	}

	UAnimSequence* DesiredAnimation = nullptr;
	if (GetCharacterMovement()->IsFalling())
	{
		DesiredAnimation = GetVelocity().Z >= 0.0f ? RifleJumpAnimation : RifleFallAnimation;
	}
	else if (GetVelocity().SizeSquared2D() > FMath::Square(10.0f))
	{
		DesiredAnimation = SelectRifleMovementAnimation();
	}
	else
	{
		DesiredAnimation = RifleIdleAnimation;
	}

	PlayRifleAnimation(DesiredAnimation, true);
}

void ACWSPlayerCharacter::UpdateWeaponRecoil(const float DeltaSeconds)
{
	if (!WeaponMesh || !bWeaponRecoilActive)
	{
		return;
	}

	WeaponRecoilElapsed = FMath::Min(WeaponRecoilElapsed + DeltaSeconds, WeaponRecoilDuration);
	const float NormalizedTime = WeaponRecoilDuration > KINDA_SMALL_NUMBER
		? WeaponRecoilElapsed / WeaponRecoilDuration
		: 1.0f;
	const float KickAlpha = NormalizedTime < 0.22f
		? FMath::InterpEaseOut(0.0f, 1.0f, NormalizedTime / 0.22f, 2.0f)
		: FMath::Square(1.0f - ((NormalizedTime - 0.22f) / 0.78f));

	FTransform RecoilTransform = WeaponRestRelativeTransform;
	const FVector LocalRecoilOffset(-7.0f * KickAlpha, 0.0f, 1.25f * KickAlpha);
	RecoilTransform.AddToTranslation(
		WeaponRestRelativeTransform.GetRotation().RotateVector(LocalRecoilOffset));
	RecoilTransform.SetRotation(
		(WeaponRestRelativeTransform.GetRotation() * FRotator(3.5f * KickAlpha, 0.0f, 0.0f).Quaternion())
		.GetNormalized());
	WeaponMesh->SetRelativeTransform(RecoilTransform);

	if (WeaponRecoilElapsed >= WeaponRecoilDuration)
	{
		WeaponMesh->SetRelativeTransform(WeaponRestRelativeTransform);
		bWeaponRecoilActive = false;
	}
}

void ACWSPlayerCharacter::PlayRifleAnimation(UAnimSequence* Animation, const bool bLooping, const float PlayRate)
{
	if (!Animation || CurrentRifleAnimation == Animation)
	{
		return;
	}

	GetMesh()->PlayAnimation(Animation, bLooping);
	if (UAnimSingleNodeInstance* SingleNodeInstance = GetMesh()->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetPlayRate(PlayRate);
	}
	CurrentRifleAnimation = Animation;
}

void ACWSPlayerCharacter::PlayWeaponFireRecoil()
{
	if (!WeaponMesh)
	{
		return;
	}

	// MM_Rifle_Fire is additive data and cannot be used as a full-body single-node
	// animation. Keep the current locomotion pose and animate only the attached gun
	// so firing never replaces the character with an invalid additive pose.
	WeaponRecoilElapsed = 0.0f;
	bWeaponRecoilActive = true;
}

void ACWSPlayerCharacter::PlayRifleAction(UAnimSequence* Animation, const float Duration)
{
	if (!Animation || !GetWorld())
	{
		return;
	}

	CurrentRifleAnimation = nullptr;
	const float SafeDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	const float PlayRate = FMath::Max(Animation->GetPlayLength() / SafeDuration, KINDA_SMALL_NUMBER);
	PlayRifleAnimation(Animation, false, PlayRate);
	RifleActionEndTime = GetWorld()->GetTimeSeconds() + SafeDuration;
}

UAnimSequence* ACWSPlayerCharacter::SelectRifleMovementAnimation() const
{
	if (RifleJogAnimations.Num() != 8)
	{
		return RifleIdleAnimation;
	}

	const FVector Direction = GetVelocity().GetSafeNormal2D();
	const float ForwardAmount = FVector::DotProduct(GetActorForwardVector(), Direction);
	const float RightAmount = FVector::DotProduct(GetActorRightVector(), Direction);
	const float DirectionDegrees = FMath::RadiansToDegrees(FMath::Atan2(RightAmount, ForwardAmount));
	const int32 DirectionIndex = FMath::FloorToInt((DirectionDegrees + 22.5f + 360.0f) / 45.0f) % 8;
	return RifleJogAnimations[DirectionIndex];
}

void ACWSPlayerCharacter::RestartLevel(const FInputActionValue& Value)
{
	if (ACWSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACWSGameMode>() : nullptr)
	{
		if (GameMode->IsWaitingForStart())
		{
			GameMode->StartGame();
		}
		else
		{
			GameMode->RestartCurrentLevel();
		}
	}
}

bool ACWSPlayerCharacter::CanUseGameplayInput() const
{
	const ACWSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACWSGameMode>() : nullptr;
	return !GameMode || GameMode->IsGameStarted();
}

void ACWSPlayerCharacter::HandleDeath(AActor* DeadActor)
{
	GetCharacterMovement()->DisableMovement();
}
