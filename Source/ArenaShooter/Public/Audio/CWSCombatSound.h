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

/**
 * 짧은 전투음을 PCM으로 합성하는 일회성 Procedural SoundWave입니다.
 * 별도 사운드 에셋 없이도 Editor/Shipping 및 자동 검증에서 같은 생성 경로를 사용합니다.
 */
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

/** 지정한 위치에 전투음을 생성하고 재생에 성공했는지 반환합니다. */
ARENASHOOTER_API bool PlayCWSCombatSound(
	const UObject* WorldContextObject,
	const FVector& Location,
	ECWSCombatSoundType SoundType,
	float VolumeMultiplier = 1.0f);
