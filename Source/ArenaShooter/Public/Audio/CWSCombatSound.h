#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "CWSCombatSound.generated.h"

enum class ECWSCombatSoundType : uint8
{
	WeaponFire,
	BulletImpact,
	EnemyAttack,
	BossExplosion
};

UCLASS()
class ARENASHOOTER_API UCWSCombatSoundWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	UCWSCombatSoundWave(const FObjectInitializer& ObjectInitializer);

	void Initialize(ECWSCombatSoundType SoundType);
	virtual float GetDuration() const override { return DurationSeconds; }
	virtual bool IsOneShot() const override { return true; }

private:
	float DurationSeconds = 0.1f;
};

ARENASHOOTER_API bool PlayCWSCombatSound(
	const UObject* WorldContextObject,
	const FVector& Location,
	ECWSCombatSoundType SoundType,
	float VolumeMultiplier = 1.0f);
