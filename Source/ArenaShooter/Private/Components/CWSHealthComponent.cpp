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
		// 모든 엔진 데미지 진입점을 이 컴포넌트의 단일 변경 함수로 모은다.
		Owner->OnTakeAnyDamage.AddDynamic(this, &UCWSHealthComponent::HandleTakeAnyDamage);
		OnHealthChanged.Broadcast(Owner, CurrentHealth, MaxHealth, nullptr);
	}
}

float UCWSHealthComponent::ApplyHealthChange(const float Delta, AActor* ChangeInstigator)
{
	// 사망 뒤 추가 피해는 무시하지만 양수 회복은 호출자가 명시적으로 허용할 수 있다.
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

	// bIsDead를 먼저 세워 중첩된 이벤트에서도 OnDeath가 정확히 한 번만 발생한다.
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

	// 피격 로그/연출에는 가능하면 실제 DamageCauser를, 없으면 Controller의 Pawn을 전달한다.
	AActor* ChangeInstigator = DamageCauser;
	if (!ChangeInstigator && InstigatedBy)
	{
		ChangeInstigator = InstigatedBy->GetPawn();
	}
	ApplyHealthChange(-Damage, ChangeInstigator);
}
