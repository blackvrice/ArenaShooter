#include "Feedback/CWSCombatBurstEffect.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACWSCombatBurstEffect::ACWSCombatBurstEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BurstMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BurstMesh"));
	BurstMesh->SetupAttachment(SceneRoot);
	BurstMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BurstMesh->SetCastShadow(false);
	BurstMesh->SetRelativeScale3D(FVector(0.04f));

	BurstLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BurstLight"));
	BurstLight->SetupAttachment(SceneRoot);
	BurstLight->SetAttenuationRadius(260.0f);
	BurstLight->SetSourceRadius(18.0f);
	BurstLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		BurstMesh->SetStaticMesh(SphereMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicShapeMaterialAsset.Succeeded())
	{
		BurstMesh->SetMaterial(0, BasicShapeMaterialAsset.Object);
	}
}

bool ACWSCombatBurstEffect::SpawnBurst(
	UWorld* World,
	const FVector& Location,
	const FLinearColor& Color,
	const float MaximumScale,
	const float Duration)
{
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;
	ACWSCombatBurstEffect* Effect = World->SpawnActor<ACWSCombatBurstEffect>(
		StaticClass(),
		Location,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!Effect)
	{
		return false;
	}

	Effect->Configure(Color, MaximumScale, Duration);
	return true;
}

void ACWSCombatBurstEffect::Configure(
	const FLinearColor& Color,
	const float MaximumScale,
	const float Duration)
{
	TargetScale = FMath::Max(MaximumScale, 0.08f);
	EffectDuration = FMath::Max(Duration, 0.08f);
	InitialLightIntensity = 4500.0f * FMath::Clamp(TargetScale / 0.32f, 1.0f, 3.0f);
	BurstLight->SetLightColor(Color);
	BurstLight->SetIntensity(InitialLightIntensity);

	if (UMaterialInstanceDynamic* Material = BurstMesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ACWSCombatBurstEffect::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedTime / EffectDuration, 0.0f, 1.0f);
	const float Pulse = FMath::Sin(Alpha * PI);
	BurstMesh->SetRelativeScale3D(FVector(FMath::Lerp(0.04f, TargetScale, Alpha)));
	BurstLight->SetIntensity(InitialLightIntensity * Pulse * (1.0f - Alpha));

	if (Alpha >= 1.0f)
	{
		Destroy();
	}
}
