#include "Audio/CWSCombatSound.h"

#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 CombatSoundSampleRate = 44100;

	float GenerateSample(const ECWSCombatSoundType SoundType, const float Time, FRandomStream& Noise)
	{
		const float WhiteNoise = Noise.GetFraction() * 2.0f - 1.0f;
		switch (SoundType)
		{
		case ECWSCombatSoundType::WeaponFire:
		{
			const float Envelope = FMath::Exp(-22.0f * Time);
			const float Body = FMath::Sin(2.0f * PI * (115.0f - 45.0f * Time) * Time);
			const float Crack = FMath::Sin(2.0f * PI * 1800.0f * Time) * FMath::Exp(-70.0f * Time);
			return (WhiteNoise * 0.7f + Body * 0.75f + Crack * 0.35f) * Envelope;
		}
		case ECWSCombatSoundType::BulletImpact:
		{
			const float Envelope = FMath::Exp(-34.0f * Time);
			const float Ring = FMath::Sin(2.0f * PI * 920.0f * Time) +
				0.5f * FMath::Sin(2.0f * PI * 1470.0f * Time);
			return (WhiteNoise * 0.55f + Ring * 0.45f) * Envelope;
		}
		case ECWSCombatSoundType::EnemyAttack:
		{
			const float Envelope = FMath::Sin(FMath::Clamp(Time / 0.22f, 0.0f, 1.0f) * PI);
			const float SweepFrequency = FMath::Lerp(520.0f, 105.0f, FMath::Clamp(Time / 0.22f, 0.0f, 1.0f));
			return (FMath::Sin(2.0f * PI * SweepFrequency * Time) * 0.65f + WhiteNoise * 0.35f) * Envelope;
		}
		case ECWSCombatSoundType::BossExplosion:
		default:
		{
			const float Envelope = FMath::Exp(-5.5f * Time);
			const float Boom = FMath::Sin(2.0f * PI * (72.0f - 28.0f * Time) * Time);
			const float Rumble = FMath::Sin(2.0f * PI * 38.0f * Time);
			return (Boom * 0.8f + Rumble * 0.4f + WhiteNoise * 0.65f) * Envelope;
		}
		}
	}

	float GetSoundDuration(const ECWSCombatSoundType SoundType)
	{
		switch (SoundType)
		{
		case ECWSCombatSoundType::WeaponFire:
			return 0.18f;
		case ECWSCombatSoundType::BulletImpact:
			return 0.14f;
		case ECWSCombatSoundType::EnemyAttack:
			return 0.24f;
		case ECWSCombatSoundType::BossExplosion:
		default:
			return 0.7f;
		}
	}
}

UCWSCombatSoundWave::UCWSCombatSoundWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumChannels = 1;
	SetSampleRate(CombatSoundSampleRate);
	SoundGroup = SOUNDGROUP_Effects;
	bLooping = false;
}

void UCWSCombatSoundWave::Initialize(const ECWSCombatSoundType SoundType)
{
	DurationSeconds = GetSoundDuration(SoundType);
	const int32 SampleCount = FMath::CeilToInt(DurationSeconds * CombatSoundSampleRate);
	TArray<int16> Samples;
	Samples.SetNumUninitialized(SampleCount);
	FRandomStream Noise(1729 + static_cast<int32>(SoundType) * 7919);
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const float Time = static_cast<float>(SampleIndex) / static_cast<float>(CombatSoundSampleRate);
		const float Sample = FMath::Clamp(GenerateSample(SoundType, Time, Noise) * 0.72f, -1.0f, 1.0f);
		Samples[SampleIndex] = static_cast<int16>(Sample * static_cast<float>(MAX_int16));
	}

	QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}

bool PlayCWSCombatSound(
	const UObject* WorldContextObject,
	const FVector& Location,
	const ECWSCombatSoundType SoundType,
	const float VolumeMultiplier)
{
	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		return false;
	}

	UCWSCombatSoundWave* SoundWave = NewObject<UCWSCombatSoundWave>(GetTransientPackage());
	if (!SoundWave)
	{
		return false;
	}
	SoundWave->Initialize(SoundType);
	const bool bQueuedAudio = SoundWave->GetAvailableAudioByteCount() > 0;
	UGameplayStatics::PlaySoundAtLocation(
		WorldContextObject,
		SoundWave,
		Location,
		FMath::Max(VolumeMultiplier, 0.0f));
	return bQueuedAudio;
}
