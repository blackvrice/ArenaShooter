#include "Components/CWSHealthComponent.h"

#include "GameFramework/Controller.h"

UCWSHealthComponent::UCWSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCWSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
	bIsDead = false;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UCWSHealthComponent::HandleTakeAnyDamage);
		OnHealthChanged.Broadcast(Owner, CurrentHealth, MaxHealth, nullptr);
	}
}

float UCWSHealthComponent::ApplyHealthChange(const float Delta, AActor* ChangeInstigator)
{
	if (bIsDead && Delta <= 0.0f)
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Delta, 0.0f, MaxHealth);
	const float AppliedDelta = CurrentHealth - OldHealth;
	if (FMath::IsNearlyZero(AppliedDelta))
	{
		return 0.0f;
	}

	AActor* Owner = GetOwner();
	OnHealthChanged.Broadcast(Owner, CurrentHealth, MaxHealth, ChangeInstigator);

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast(Owner);
	}

	return AppliedDelta;
}

void UCWSHealthComponent::Kill(AActor* ChangeInstigator)
{
	ApplyHealthChange(-MaxHealth, ChangeInstigator);
}

void UCWSHealthComponent::SetMaxHealth(const float NewMaxHealth, const bool bResetCurrentHealth)
{
	MaxHealth = FMath::Max(NewMaxHealth, 1.0f);
	CurrentHealth = bResetCurrentHealth ? MaxHealth : FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
	bIsDead = CurrentHealth <= 0.0f;
}

float UCWSHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UCWSHealthComponent::HandleTakeAnyDamage(
	AActor* DamagedActor,
	const float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return;
	}

	AActor* ChangeInstigator = DamageCauser;
	if (!ChangeInstigator && InstigatedBy)
	{
		ChangeInstigator = InstigatedBy->GetPawn();
	}
	ApplyHealthChange(-Damage, ChangeInstigator);
}
