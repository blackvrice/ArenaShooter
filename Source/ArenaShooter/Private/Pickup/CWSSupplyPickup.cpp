#include "Pickup/CWSSupplyPickup.h"

#include "Components/CWSHealthComponent.h"
#include "Components/CWSHitscanWeaponComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/CWSPlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSSupply, Log, All);

ACWSSupplyPickup::ACWSSupplyPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	SetRootComponent(CollectionSphere);
	CollectionSphere->InitSphereRadius(85.0f);
	CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	SupplyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SupplyMesh"));
	SupplyMesh->SetupAttachment(CollectionSphere);
	SupplyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SupplyMesh->SetRelativeScale3D(FVector(0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		SupplyMesh->SetStaticMesh(CubeMesh.Object);
	}

	SupplyLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SupplyLabel"));
	SupplyLabel->SetupAttachment(CollectionSphere);
	SupplyLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	SupplyLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	SupplyLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	SupplyLabel->SetWorldSize(28.0f);
	SupplyLabel->SetTextRenderColor(FColor::White);
}

void ACWSSupplyPickup::BeginPlay()
{
	Super::BeginPlay();
	BaseLocation = GetActorLocation();
	CollectionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACWSSupplyPickup::HandleOverlap);
	UpdatePresentation();
}

void ACWSSupplyPickup::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RunningTime += DeltaSeconds;
	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaSeconds, 0.0f));
	FVector NewLocation = BaseLocation;
	NewLocation.Z += FMath::Sin(RunningTime * BobSpeed) * BobHeight;
	SetActorLocation(NewLocation);
}

void ACWSSupplyPickup::ConfigureSupply(const ECWSSupplyType NewSupplyType)
{
	SupplyType = NewSupplyType;
	UpdatePresentation();
}

bool ACWSSupplyPickup::TryCollect(AActor* Collector)
{
	ACWSPlayerCharacter* Player = Cast<ACWSPlayerCharacter>(Collector);
	if (bCollected || !Player)
	{
		return false;
	}

	bool bApplied = false;
	if (SupplyType == ECWSSupplyType::Health)
	{
		if (UCWSHealthComponent* Health = Player->GetHealthComponent())
		{
			bApplied = Health->ApplyHealthChange(HealthAmount, this) > 0.0f;
		}
	}
	else if (UCWSHitscanWeaponComponent* Weapon = Player->GetWeaponComponent())
	{
		bApplied = Weapon->AddReserveAmmo(AmmoAmount) > 0;
	}

	// 체력/예비 탄약이 이미 가득 찼다면 나중에 다시 쓸 수 있도록 Pickup을 남긴다.
	if (!bApplied)
	{
		return false;
	}

	bCollected = true;
	OnSupplyCollected.Broadcast(Player, SupplyType);
	UE_LOG(
		LogCWSSupply,
		Display,
		TEXT("Player collected %s supply."),
		SupplyType == ECWSSupplyType::Health ? TEXT("health") : TEXT("ammo"));
	Destroy();
	return true;
}

void ACWSSupplyPickup::HandleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TryCollect(OtherActor);
}

void ACWSSupplyPickup::UpdatePresentation()
{
	if (!SupplyLabel || !SupplyMesh)
	{
		return;
	}

	const bool bHealthSupply = SupplyType == ECWSSupplyType::Health;
	SupplyLabel->SetText(FText::FromString(bHealthSupply ? TEXT("HEALTH +40") : TEXT("AMMO +90")));
	SupplyLabel->SetTextRenderColor(bHealthSupply ? FColor::Green : FColor::Yellow);
	SupplyMesh->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		bHealthSupply ? FVector(0.05f, 0.8f, 0.15f) : FVector(0.95f, 0.65f, 0.05f));
}
