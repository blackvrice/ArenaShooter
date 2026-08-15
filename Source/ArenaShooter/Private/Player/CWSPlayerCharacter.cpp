#include "Player/CWSPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/CWSGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

ACWSPlayerCharacter::ACWSPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UCWSHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(100.0f);
	WeaponComponent = CreateDefaultSubobject<UCWSHitscanWeaponComponent>(TEXT("WeaponComponent"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PlayerMeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (PlayerMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(PlayerMeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> PlayerAnimClass(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (PlayerAnimClass.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(PlayerAnimClass.Class);
	}

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
	CombatMappingContext = NewObject<UInputMappingContext>(this, TEXT("CWSCombatMappingContext"));
	if (FireAction)
	{
		CombatMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	}
	if (ReloadAction)
	{
		CombatMappingContext->MapKey(ReloadAction, EKeys::RightMouseButton);
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
	if (!Controller)
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
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void ACWSPlayerCharacter::StartJump(const FInputActionValue& Value)
{
	Jump();
}

void ACWSPlayerCharacter::StopJump(const FInputActionValue& Value)
{
	StopJumping();
}

void ACWSPlayerCharacter::Fire(const FInputActionValue& Value)
{
	if (HealthComponent->IsAlive())
	{
		WeaponComponent->TryFire();
	}
}

void ACWSPlayerCharacter::Reload(const FInputActionValue& Value)
{
	if (HealthComponent->IsAlive())
	{
		WeaponComponent->Reload();
	}
}

void ACWSPlayerCharacter::RestartLevel(const FInputActionValue& Value)
{
	if (ACWSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACWSGameMode>() : nullptr)
	{
		GameMode->RestartCurrentLevel();
	}
}

void ACWSPlayerCharacter::HandleDeath(AActor* DeadActor)
{
	GetCharacterMovement()->DisableMovement();
}
