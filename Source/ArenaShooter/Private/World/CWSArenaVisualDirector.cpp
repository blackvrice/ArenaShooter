#include "World/CWSArenaVisualDirector.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSArenaVisuals, Log, All);

namespace
{
	const FLinearColor RingColor(0.02f, 0.55f, 1.0f);
	const FLinearColor CoverColor(0.035f, 0.09f, 0.16f);
	const FLinearColor NorthColor(0.05f, 0.45f, 1.0f);
	const FLinearColor SouthColor(1.0f, 0.08f, 0.04f);
	const FLinearColor EastColor(1.0f, 0.55f, 0.03f);
	const FLinearColor WestColor(0.65f, 0.08f, 1.0f);
}

ACWSArenaVisualDirector::ACWSArenaVisualDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CenterRing = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CenterRing"));
	CoverBlocks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CoverBlocks"));
	NorthGate = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NorthGate"));
	SouthGate = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SouthGate"));
	EastGate = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EastGate"));
	WestGate = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WestGate"));
	ConfigureMeshComponent(CenterRing, false);
	ConfigureMeshComponent(CoverBlocks, true);
	ConfigureMeshComponent(NorthGate, false);
	ConfigureMeshComponent(SouthGate, false);
	ConfigureMeshComponent(EastGate, false);
	ConfigureMeshComponent(WestGate, false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for (UInstancedStaticMeshComponent* Component :
		{CenterRing, CoverBlocks, NorthGate, SouthGate, EastGate, WestGate})
	{
		if (CubeMeshAsset.Succeeded())
		{
			Component->SetStaticMesh(CubeMeshAsset.Object);
		}
		if (BasicShapeMaterialAsset.Succeeded())
		{
			Component->SetMaterial(0, BasicShapeMaterialAsset.Object);
		}
	}

	constexpr int32 RingSegments = 24;
	constexpr float RingRadius = 720.0f;
	for (int32 Index = 0; Index < RingSegments; ++Index)
	{
		const float AngleRadians = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(RingSegments);
		const FVector Location(FMath::Cos(AngleRadians) * RingRadius, FMath::Sin(AngleRadians) * RingRadius, 4.0f);
		const FRotator Rotation(0.0f, FMath::RadiansToDegrees(AngleRadians) + 90.0f, 0.0f);
		CenterRing->AddInstance(FTransform(Rotation, Location, FVector(1.85f, 0.08f, 0.035f)));
	}

	const TArray<FTransform> CoverTransforms = {
		FTransform(FRotator(0.0f, 15.0f, 0.0f), FVector(-720.0f, 520.0f, 55.0f), FVector(2.6f, 0.55f, 1.1f)),
		FTransform(FRotator(0.0f, -15.0f, 0.0f), FVector(720.0f, 520.0f, 55.0f), FVector(2.6f, 0.55f, 1.1f)),
		FTransform(FRotator(0.0f, -15.0f, 0.0f), FVector(-720.0f, -520.0f, 55.0f), FVector(2.6f, 0.55f, 1.1f)),
		FTransform(FRotator(0.0f, 15.0f, 0.0f), FVector(720.0f, -520.0f, 55.0f), FVector(2.6f, 0.55f, 1.1f)),
		FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-1120.0f, 0.0f, 42.0f), FVector(1.8f, 0.45f, 0.84f)),
		FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(1120.0f, 0.0f, 42.0f), FVector(1.8f, 0.45f, 0.84f)),
		FTransform(FRotator::ZeroRotator, FVector(0.0f, -1120.0f, 42.0f), FVector(1.8f, 0.45f, 0.84f)),
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 1120.0f, 42.0f), FVector(1.8f, 0.45f, 0.84f))
	};
	for (const FTransform& Transform : CoverTransforms)
	{
		CoverBlocks->AddInstance(Transform);
	}

	auto AddGatePair = [](UInstancedStaticMeshComponent* Gate, const FVector& First, const FVector& Second)
	{
		const FVector Scale(0.22f, 0.22f, 2.8f);
		Gate->AddInstance(FTransform(FRotator::ZeroRotator, First, Scale));
		Gate->AddInstance(FTransform(FRotator::ZeroRotator, Second, Scale));
	};
	AddGatePair(NorthGate, FVector(-360.0f, 1760.0f, 140.0f), FVector(360.0f, 1760.0f, 140.0f));
	AddGatePair(SouthGate, FVector(-360.0f, -1760.0f, 140.0f), FVector(360.0f, -1760.0f, 140.0f));
	AddGatePair(EastGate, FVector(1760.0f, -360.0f, 140.0f), FVector(1760.0f, 360.0f, 140.0f));
	AddGatePair(WestGate, FVector(-1760.0f, -360.0f, 140.0f), FVector(-1760.0f, 360.0f, 140.0f));

	NorthGateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("NorthGateLight"));
	SouthGateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SouthGateLight"));
	EastGateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EastGateLight"));
	WestGateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WestGateLight"));
	const TArray<TPair<UPointLightComponent*, FVector>> GateLights = {
		{NorthGateLight, FVector(0.0f, 1680.0f, 210.0f)},
		{SouthGateLight, FVector(0.0f, -1680.0f, 210.0f)},
		{EastGateLight, FVector(1680.0f, 0.0f, 210.0f)},
		{WestGateLight, FVector(-1680.0f, 0.0f, 210.0f)}
	};
	for (const TPair<UPointLightComponent*, FVector>& GateLight : GateLights)
	{
		GateLight.Key->SetupAttachment(SceneRoot);
		GateLight.Key->SetRelativeLocation(GateLight.Value);
		GateLight.Key->SetIntensity(2200.0f);
		GateLight.Key->SetAttenuationRadius(650.0f);
		GateLight.Key->SetSourceRadius(25.0f);
		GateLight.Key->SetCastShadows(false);
	}
}

void ACWSArenaVisualDirector::BeginPlay()
{
	Super::BeginPlay();
	bPresentationReady =
		ApplyColor(CenterRing, RingColor) && ApplyColor(CoverBlocks, CoverColor) &&
		ApplyColor(NorthGate, NorthColor) && ApplyColor(SouthGate, SouthColor) &&
		ApplyColor(EastGate, EastColor) && ApplyColor(WestGate, WestColor);
	NorthGateLight->SetLightColor(NorthColor);
	SouthGateLight->SetLightColor(SouthColor);
	EastGateLight->SetLightColor(EastColor);
	WestGateLight->SetLightColor(WestColor);
	UE_LOG(
		LogCWSArenaVisuals,
		Display,
		TEXT("CWS_ARENA_VISUALS_READY: Ring=%d Cover=%d GateBeacons=%d Materials=%s"),
		GetCenterRingSegmentCount(),
		GetCoverCount(),
		GetGateBeaconCount(),
		bPresentationReady ? TEXT("true") : TEXT("false"));
}

int32 ACWSArenaVisualDirector::GetCenterRingSegmentCount() const
{
	return CenterRing ? CenterRing->GetInstanceCount() : 0;
}

int32 ACWSArenaVisualDirector::GetCoverCount() const
{
	return CoverBlocks ? CoverBlocks->GetInstanceCount() : 0;
}

int32 ACWSArenaVisualDirector::GetGateBeaconCount() const
{
	return (NorthGate ? NorthGate->GetInstanceCount() : 0) +
		(SouthGate ? SouthGate->GetInstanceCount() : 0) +
		(EastGate ? EastGate->GetInstanceCount() : 0) +
		(WestGate ? WestGate->GetInstanceCount() : 0);
}

bool ACWSArenaVisualDirector::HasBlockingCover() const
{
	return CoverBlocks &&
		CoverBlocks->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics &&
		CoverBlocks->GetCollisionProfileName() == TEXT("BlockAll");
}

void ACWSArenaVisualDirector::ConfigureMeshComponent(
	UInstancedStaticMeshComponent* Component,
	const bool bBlocksMovement)
{
	Component->SetupAttachment(SceneRoot);
	Component->SetCollisionEnabled(bBlocksMovement
		? ECollisionEnabled::QueryAndPhysics
		: ECollisionEnabled::NoCollision);
	Component->SetCollisionProfileName(bBlocksMovement ? TEXT("BlockAll") : TEXT("NoCollision"));
	Component->SetCanEverAffectNavigation(bBlocksMovement);
}

bool ACWSArenaVisualDirector::ApplyColor(
	UInstancedStaticMeshComponent* Component,
	const FLinearColor& Color)
{
	if (!Component)
	{
		return false;
	}
	if (UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		return true;
	}
	return false;
}
