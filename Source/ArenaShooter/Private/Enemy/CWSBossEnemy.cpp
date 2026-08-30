#include "Enemy/CWSBossEnemy.h"

#include "Audio/CWSCombatSound.h"
#include "Components/CapsuleComponent.h"
#include "Components/CWSHealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCWSBoss, Log, All);

ACWSBossEnemy::ACWSBossEnemy()
{
	EnemyType = ECWSEnemyType::Boss;
	HealthComponent->SetMaxHealth(BossMaxHealth);
	GetCharacterMovement()->MaxWalkSpeed = 260.0f;
	AttackDamage = GroundSlamDamage;
	AttackRange = GroundSlamRadius;
	AttackInterval = 2.4f;
	GetCapsuleComponent()->InitCapsuleSize(60.0f, 130.0f);

	// Keep the base enemy's verified lightweight visual profile until the dedicated
	// boss mesh can be rebuilt. Loading a corrupt derived-data payload here crashes
	// the editor before the map opens, so size and the boss marker provide the
	// archetype distinction without touching that payload.
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -130.0f));
	GetMesh()->SetRelativeScale3D(FVector(1.65f));

	BossAuraRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossAuraRing"));
	BossAuraRing->SetupAttachment(GetCapsuleComponent());
	BossAuraRing->SetRelativeLocation(FVector(0.0f, 0.0f, -124.0f));
	BossAuraRing->SetRelativeScale3D(FVector(1.65f, 1.65f, 0.035f));
	BossAuraRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossAuraRing->SetCastShadow(false);

	BossCrown = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossCrown"));
	BossCrown->SetupAttachment(GetCapsuleComponent());
	BossCrown->SetRelativeLocation(FVector(0.0f, 0.0f, 224.0f));
	BossCrown->SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
	BossCrown->SetRelativeScale3D(FVector(0.46f, 0.46f, 0.34f));
	BossCrown->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossCrown->SetCastShadow(false);

	BossPhaseLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BossPhaseLight"));
	BossPhaseLight->SetupAttachment(GetCapsuleComponent());
	BossPhaseLight->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
	BossPhaseLight->SetIntensity(5000.0f);
	BossPhaseLight->SetAttenuationRadius(480.0f);
	BossPhaseLight->SetSourceRadius(28.0f);
	BossPhaseLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshAsset(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CylinderMeshAsset.Succeeded())
	{
		BossAuraRing->SetStaticMesh(CylinderMeshAsset.Object);
	}
	if (ConeMeshAsset.Succeeded())
	{
		BossCrown->SetStaticMesh(ConeMeshAsset.Object);
	}
	if (BasicShapeMaterialAsset.Succeeded())
	{
		BossAuraRing->SetMaterial(0, BasicShapeMaterialAsset.Object);
		BossCrown->SetMaterial(0, BasicShapeMaterialAsset.Object);
	}
}

void ACWSBossEnemy::BeginPlay()
{
	Super::BeginPlay();
	BossAuraMaterial = BossAuraRing->CreateDynamicMaterialInstance(0);
	BossCrownMaterial = BossCrown->CreateDynamicMaterialInstance(0);
	bBossPresentationReady = BossAuraMaterial && BossCrownMaterial &&
		BossAuraRing->GetStaticMesh() && BossCrown->GetStaticMesh() && BossPhaseLight;
	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ACWSBossEnemy::HandleBossHealthChanged);
	UpdateBossPhase(HealthComponent->GetHealthPercent());
	UE_LOG(
		LogCWSBoss,
		Display,
		TEXT("Boss spawned with %.0f health. Presentation=%s"),
		HealthComponent->GetMaxHealth(),
		bBossPresentationReady ? TEXT("ready") : TEXT("missing"));
}

bool ACWSBossEnemy::TryAttack(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetActor) || !HealthComponent->IsAlive() || World->GetTimeSeconds() < NextPatternTime)
	{
		return false;
	}

	ECWSBossPattern Pattern = ECWSBossPattern::GroundSlam;
	if (BossPhase == ECWSBossPhase::FinalPhase || (BossPhase == ECWSBossPhase::PhaseTwo && bUseShockwaveNext))
	{
		Pattern = ECWSBossPattern::Shockwave;
	}

	const bool bExecuted = Pattern == ECWSBossPattern::Shockwave
		? ExecuteShockwave(TargetActor)
		: ExecuteGroundSlam(TargetActor);
	if (!bExecuted)
	{
		return false;
	}

	if (BossPhase == ECWSBossPhase::PhaseTwo)
	{
		bUseShockwaveNext = !bUseShockwaveNext;
	}
	const float PhaseInterval = BossPhase == ECWSBossPhase::PhaseOne
		? 2.4f
		: BossPhase == ECWSBossPhase::PhaseTwo ? 1.8f : 1.1f;
	NextPatternTime = World->GetTimeSeconds() + PhaseInterval;
	PlayAttackAnimation();
	if (PlayCWSCombatSound(this, GetActorLocation(), ECWSCombatSoundType::BossExplosion, 1.2f))
	{
		++ExplosionSoundPlayCount;
	}
	RecordPattern(Pattern);
	return true;
}

FString ACWSBossEnemy::GetBossPhaseLabel() const
{
	switch (BossPhase)
	{
	case ECWSBossPhase::PhaseTwo:
		return TEXT("PHASE 2");
	case ECWSBossPhase::FinalPhase:
		return TEXT("FINAL PHASE");
	default:
		return TEXT("PHASE 1");
	}
}

FString ACWSBossEnemy::GetLastPatternLabel() const
{
	switch (LastPattern)
	{
	case ECWSBossPattern::GroundSlam:
		return TEXT("GROUND SLAM");
	case ECWSBossPattern::Shockwave:
		return TEXT("SHOCKWAVE");
	default:
		return TEXT("READY");
	}
}

void ACWSBossEnemy::HandleBossHealthChanged(
	AActor* DamagedActor,
	const float CurrentHealth,
	const float MaxHealth,
	AActor* ChangeInstigator)
{
	UpdateBossPhase(MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f);
}

void ACWSBossEnemy::UpdateBossPhase(const float HealthPercent)
{
	ECWSBossPhase NewPhase = ECWSBossPhase::PhaseOne;
	if (HealthPercent <= 0.33f)
	{
		NewPhase = ECWSBossPhase::FinalPhase;
	}
	else if (HealthPercent <= 0.66f)
	{
		NewPhase = ECWSBossPhase::PhaseTwo;
	}

	if (BossPhase == NewPhase)
	{
		ApplyBossPhasePresentation();
		return;
	}

	BossPhase = NewPhase;
	GetCharacterMovement()->MaxWalkSpeed = BossPhase == ECWSBossPhase::PhaseTwo ? 320.0f : 380.0f;
	NextPatternTime = 0.0f;
	ApplyBossPhasePresentation();
	OnBossPhaseChanged.Broadcast(BossPhase);
	UE_LOG(LogCWSBoss, Display, TEXT("Boss entered %s."), *GetBossPhaseLabel());
}

void ACWSBossEnemy::ApplyBossPhasePresentation()
{
	FLinearColor PhaseColor(0.55f, 0.03f, 1.0f);
	float LightIntensity = 5000.0f;
	float AuraScale = 1.65f;
	if (BossPhase == ECWSBossPhase::PhaseTwo)
	{
		PhaseColor = FLinearColor(1.0f, 0.22f, 0.015f);
		LightIntensity = 6800.0f;
		AuraScale = 1.85f;
	}
	else if (BossPhase == ECWSBossPhase::FinalPhase)
	{
		PhaseColor = FLinearColor(1.0f, 0.015f, 0.05f);
		LightIntensity = 9000.0f;
		AuraScale = 2.05f;
	}

	if (BossAuraMaterial)
	{
		BossAuraMaterial->SetVectorParameterValue(TEXT("Color"), PhaseColor);
	}
	if (BossCrownMaterial)
	{
		BossCrownMaterial->SetVectorParameterValue(TEXT("Color"), PhaseColor * 1.35f);
	}
	if (BossAuraRing)
	{
		BossAuraRing->SetRelativeScale3D(FVector(AuraScale, AuraScale, 0.035f));
	}
	if (BossPhaseLight)
	{
		BossPhaseLight->SetLightColor(PhaseColor);
		BossPhaseLight->SetIntensity(LightIntensity);
		BossPhaseLight->SetAttenuationRadius(BossPhase == ECWSBossPhase::FinalPhase ? 650.0f : 500.0f);
	}

	UE_LOG(
		LogCWSBoss,
		Display,
		TEXT("CWS_BOSS_PRESENTATION_PHASE: %s Color=(%.2f,%.2f,%.2f) AuraScale=%.2f"),
		*GetBossPhaseLabel(),
		PhaseColor.R,
		PhaseColor.G,
		PhaseColor.B,
		AuraScale);
}

bool ACWSBossEnemy::ExecuteGroundSlam(AActor* TargetActor)
{
	if (FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(GroundSlamRadius))
	{
		return false;
	}
	UGameplayStatics::ApplyDamage(TargetActor, GroundSlamDamage, GetController(), this, UDamageType::StaticClass());
	return true;
}

bool ACWSBossEnemy::ExecuteShockwave(AActor* TargetActor)
{
	const FVector Offset = TargetActor->GetActorLocation() - GetActorLocation();
	if (Offset.SizeSquared() > FMath::Square(ShockwaveRadius))
	{
		return false;
	}

	UGameplayStatics::ApplyDamage(TargetActor, ShockwaveDamage, GetController(), this, UDamageType::StaticClass());
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		const FVector LaunchDirection = Offset.GetSafeNormal2D();
		TargetCharacter->LaunchCharacter(LaunchDirection * ShockwaveStrength + FVector(0.0f, 0.0f, 350.0f), true, true);
	}
	return true;
}

void ACWSBossEnemy::RecordPattern(const ECWSBossPattern Pattern)
{
	LastPattern = Pattern;
	++PatternExecutionCount;
	OnBossPatternExecuted.Broadcast(Pattern);
	UE_LOG(LogCWSBoss, Display, TEXT("Boss executed %s in %s."), *GetLastPatternLabel(), *GetBossPhaseLabel());
}
