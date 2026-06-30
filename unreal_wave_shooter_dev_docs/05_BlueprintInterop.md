# 05. Blueprint Interop

## 1. 기본 원칙

C++은 시스템 로직을 담당하고, Blueprint는 시각적 연결과 빠른 조정을 담당한다.

## 2. C++에서 구현할 것

| 기능 | 이유 |
|---|---|
| HealthComponent | 재사용성이 높음 |
| WaveManager | 게임 핵심 규칙 |
| SpawnPoint | 데이터 기반 처리 필요 |
| EnemyBase | 공통 적 로직 |
| Projectile | 충돌/데미지 로직 |
| WeaponBase | 무기 확장성 |
| GameMode/GameState | 게임 흐름 제어 |

## 3. Blueprint에서 구현할 것

| 기능 | 이유 |
|---|---|
| 캐릭터 Mesh 연결 | 에셋 교체 쉬움 |
| 애니메이션 Blueprint | 시각 요소 중심 |
| 사운드/VFX | 반복 조정 쉬움 |
| UI 디자인 | 배치 수정 쉬움 |
| 맵 배치 | 레벨 디자인 중심 |
| DataTable 값 조정 | 밸런싱 쉬움 |

## 4. BlueprintCallable 권장 함수

```cpp
UFUNCTION(BlueprintCallable)
void StartRound(int32 RoundNumber);

UFUNCTION(BlueprintCallable)
float GetHealthPercent() const;

UFUNCTION(BlueprintCallable)
int32 GetCurrentRound() const;

UFUNCTION(BlueprintCallable)
int32 GetRemainingEnemyCount() const;
```

## 5. BlueprintImplementableEvent 권장

```cpp
UFUNCTION(BlueprintImplementableEvent)
void OnRoundStartedBP(int32 RoundNumber);

UFUNCTION(BlueprintImplementableEvent)
void OnRoundClearedBP(int32 RoundNumber);

UFUNCTION(BlueprintImplementableEvent)
void OnBossSpawnedBP();

UFUNCTION(BlueprintImplementableEvent)
void OnEnemyDeathBP();
```

## 6. 에디터에서 배치할 Actor

- BP_PlayerCharacter
- BP_WaveManager
- BP_SpawnPoint_North
- BP_SpawnPoint_South
- BP_SpawnPoint_East
- BP_SpawnPoint_West
- BP_SpawnPoint_NorthEast
- BP_SpawnPoint_NorthWest
- BP_SpawnPoint_SouthEast
- BP_SpawnPoint_SouthWest
- BP_SpawnPoint_CenterBoss
- BP_HealthPickup
- BP_AmmoPickup

## 7. Blueprint 변수 노출 기준

아래 값은 `EditAnywhere`, `BlueprintReadWrite` 또는 `BlueprintReadOnly`로 노출하면 좋다.

- 적 체력
- 이동속도
- 공격력
- 스폰 간격
- 라운드 딜레이
- 아이템 회복량
- 탄약 보급량
- 보스 체력
- 보스 패턴 쿨타임
