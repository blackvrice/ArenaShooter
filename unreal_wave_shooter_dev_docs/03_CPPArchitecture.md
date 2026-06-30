# 03. C++ Architecture

## 1. 목표 구조

C++에서 핵심 시스템을 만들고, Blueprint에서는 에셋 연결과 연출을 담당한다.

```text
C++ Core
├─ Player
├─ Weapon
├─ Health
├─ Enemy
├─ Wave
├─ Spawn
├─ Pickup
└─ UI Interface

Blueprint
├─ Mesh / Animation
├─ VFX / SFX
├─ Level 배치
├─ Widget 디자인
└─ DataTable 입력
```

## 2. 주요 클래스 목록

| 클래스 | 부모 클래스 | 역할 |
|---|---|---|
| ACWSPlayerCharacter | ACharacter | 플레이어 이동, 조준, 사격 입력 |
| ACWSPlayerController | APlayerController | 입력 매핑, UI 생성 |
| UCWSHealthComponent | UActorComponent | 체력, 데미지, 사망 이벤트 |
| ACWSWeaponBase | AActor | 무기 기본 클래스 |
| ACWSProjectile | AActor | 투사체, 충돌, 데미지 |
| ACWSEnemyBase | ACharacter | 적 기본 클래스 |
| ACWSEnemyAIController | AAIController | 적 AI 제어 |
| ACWSBossEnemy | ACWSEnemyBase | 보스 전용 패턴 |
| ACWSSpawnPoint | AActor | 방향별 스폰 위치 |
| ACWSWaveManager | AActor | 라운드 진행, 적 스폰, 클리어 판정 |
| ACWSPickupBase | AActor | 아이템 기본 클래스 |
| ACWSHealthPickup | ACWSPickupBase | 회복 아이템 |
| ACWSAmmoPickup | ACWSPickupBase | 탄약 보급 |
| ACWSGameMode | AGameModeBase | 게임 규칙 |
| ACWSGameState | AGameStateBase | 라운드/점수 상태 공유 |

## 3. 네이밍 규칙

프로젝트 접두사 예시: `CWS` = Central Wave Shooter

| 타입 | 접두사 예시 |
|---|---|
| Actor Class | ACWS |
| Component | UCWS |
| Struct | FCWS |
| Enum | ECWS |
| DataTable Row | FCWS...Row |

## 4. 핵심 이벤트 흐름

```text
GameMode BeginPlay
→ WaveManager Initialize
→ SpawnPoint 등록
→ StartRound(1)
→ Spawn enemies by DataTable
→ Enemy 사망 시 WaveManager에 보고
→ RemainingEnemyCount == 0
→ RoundClear
→ 다음 라운드 또는 BossRound
```

## 5. HealthComponent 설계

### 변수

- MaxHealth
- CurrentHealth
- bIsDead

### 함수

- ApplyHealthChange(float Delta)
- TakeDamageFromActor(AActor* DamageCauser, float DamageAmount)
- IsAlive()
- GetHealthPercent()

### Delegate

- OnHealthChanged
- OnDeath

## 6. WaveManager 설계

### 변수

- CurrentRound
- RemainingEnemyCount
- ActiveSpawnPoints
- RoundDataTable
- SpawnInterval
- bRoundInProgress

### 함수

- StartRound(int32 RoundNumber)
- EndRound()
- SpawnEnemyFromDirection(ECWSSpawnDirection Direction)
- RegisterEnemy(ACWSEnemyBase* Enemy)
- HandleEnemyDeath(ACWSEnemyBase* Enemy)
- StartBossRound()

## 7. SpawnPoint 설계

### 변수

- SpawnDirection
- SpawnRadius
- bIsEnabled

### 함수

- GetSpawnTransform()
- CanSpawn()
- SpawnEnemy(TSubclassOf<ACWSEnemyBase> EnemyClass)

## 8. EnemyBase 설계

### 변수

- EnemyType
- AttackDamage
- AttackRange
- MoveSpeed
- ScoreValue

### 함수

- InitializeFromData()
- StartChasePlayer()
- Attack()
- Die()

## 9. 확장 가능 구조

나중에 아래 기능을 추가하기 쉽도록 만든다.

- 적 타입 추가
- 무기 타입 추가
- 난이도 배율
- 보스 패턴 추가
- 협동 멀티플레이
- 미니맵/레이더
