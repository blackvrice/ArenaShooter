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

| 클래스 | 부모 클래스 | 역할 | 상태 |
|---|---|---|---|
| `ACWSPlayerCharacter` | `ACharacter` | 이동, 카메라, 사격/재장전 입력 | 구현 |
| `UCWSHealthComponent` | `UActorComponent` | 체력, 엔진 데미지 수신, 사망 이벤트 | 구현 |
| `UCWSHitscanWeaponComponent` | `UActorComponent` | 히트스캔, 탄약, 발사 간격, 포인트 데미지 | 구현 |
| `ACWSEnemyBase` | `ACharacter` | 기본 적 체력, 이동속도, 근접 공격과 사망 | 구현 |
| `ACWSEnemyAIController` | `AAIController` | 플레이어 추적 및 공격 거리 제어 | 구현 |
| `ACWSSpawnPoint` | `AActor` | 방향별 스폰 위치 | 구현 |
| `ACWSWaveManager` | `AActor` | 라운드 진행, 적 스폰, 사망/클리어 판정 | 구현 |
| `ACWSGameMode` | `AGameModeBase` | Pawn/HUD 지정, 게임 오버·클리어·레벨 재시작, 런타임 스모크 테스트 | 구현 |
| `ACWSHUD` | `AHUD` | 조준점, 체력, 탄약, 라운드, 남은 적 표시 | 구현 |
| `ACWSBossEnemy` | `ACWSEnemyBase` | 3단계 페이즈, Ground Slam, 넉백 Shockwave | 구현 |
| Pickup 전용 클래스 | - | 회복/탄약 보급 | 후속 작업 |

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
→ 라운드 정의로 방향별 적 생성
→ HealthComponent OnDeath가 WaveManager에 보고
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

- ApplyHealthChange(float Delta, AActor* InstigatorActor)
- Kill(AActor* InstigatorActor)
- IsAlive()
- GetHealthPercent()

### Delegate

- OnHealthChanged
- OnDeath

## 6. WaveManager 설계

### 변수

- CurrentRound
- PendingSpawns
- AliveEnemies
- CachedSpawnPoints
- Rounds
- bRoundInProgress

### 함수

- StartRound(int32 RoundNumber)
- StartWaveSystem()
- StopWaveSystem()
- GetRemainingEnemyCount()
- HandleSpawnedEnemyDeath(AActor* DeadActor)
- HandleSpawnedEnemyDestroyed(AActor* DestroyedActor)

### 현재 구현

- `ACWSWaveManager`가 Round 1~5 기본 데이터를 소유한다.
- 방향 그룹을 라운드 로빈 순서로 큐에 넣어 한 방향에 스폰이 몰리지 않게 한다.
- 생성된 적의 `OnDeath` 이벤트로 즉시 남은 적을 갱신하고 `OnDestroyed`를 안전망으로 유지한다.
- `OnRoundStarted`, `OnRemainingEnemyCountChanged`, `OnRoundCleared`, `OnAllRoundsCompleted` Blueprint delegate를 제공한다.
- 기본 적 슬롯은 네이티브 `ACWSEnemyBase`, Round 5 Center 보스 슬롯은 `ACWSBossEnemy`를 사용한다.
- Boss 슬롯 생성 시 `OnBossSpawned`를 브로드캐스트해 HUD, VFX, 사운드가 연결될 수 있다.

## 7. SpawnPoint 설계

### 변수

- SpawnDirection
- SpawnRadius
- bIsEnabled

### 함수

- GetSpawnTransform()
- CanSpawn()
- SpawnEnemy(TSubclassOf<ACWSEnemyBase> EnemyClass)

현재 `ACWSSpawnPoint`는 방향 태그와 Transform을 제공하고, 실제 Spawn 호출은 `ACWSWaveManager`가 담당한다.

## 8. EnemyBase 설계

`ACWSEnemyBase`는 체력 60, 이동속도 350, 근접 공격 거리/데미지/간격을 제공한다. `ACWSEnemyAIController`가 플레이어를 NavMesh로 추적하고 공격 거리 안에서 `ApplyDamage`를 호출한다. 사망하면 이동과 충돌을 끄고 웨이브 매니저에 `OnDeath`를 전달한다.

## 9. 전투 흐름 런타임 검증

- `run_build_playable_round_one.ps1 -InspectOnly`: PlayerStart, NavMesh Bounds, GameMode, 적 클래스와 Map Check 검사
- `run_build_wave_spawning.ps1 -InspectOnly`: 9개 스폰 지점과 `8 / 16 / 24 / 34 / 15` 라운드 수량 검사
- `run_round_one_smoke.ps1`: 실제 `TryFire()` 히트스캔 피격/사망, 적 NavMesh 이동, Round 1 클리어, 플레이어 피격 사망, 웨이브 정지, 현재 레벨 재시작 검사
- `run_round_one_smoke.ps1 -AllRounds`: 실제 게임 월드에서 Round 1~5의 97개 스폰과 각 라운드 클리어, 최종 게임 클리어 검사
- 전체 라운드 검증은 전용 Boss 클래스, 체력 1200, 최종 페이즈 전환, Ground Slam과 Shockwave 피해/넉백 경로도 함께 검사한다.
- Warm DDC 직렬화 오류가 감지되면 스모크 러너가 격리된 Cold DDC로 한 번 자동 재시도한다.
- `run_repair_combat_input.ps1 -InspectOnly`: `IMC_Combat`의 액션이 없는 손상 매핑 검사

적 Capsule은 `ECC_Visibility`를 차단하므로 플레이어 무기의 Visibility 채널 라인트레이스가 실제 적에게 도달한다. 플레이어 사망 시 `ACWSGameMode`가 `ACWSWaveManager::StopWaveSystem()`을 호출하고 HUD에 게임 오버와 Enter 재시작 안내를 표시한다.

## 10. 확장 가능 구조

나중에 아래 기능을 추가하기 쉽도록 만든다.

- 적 타입 추가
- 무기 타입 추가
- 난이도 배율
- 보스 패턴 추가
- 협동 멀티플레이
- 미니맵/레이더
