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
| `UCWSHitscanWeaponComponent` | `UActorComponent` | 히트스캔, 탄창/예비 탄약, 시간 기반 재장전, 발사 간격, 포인트 데미지와 피격 Niagara | 구현 |
| `ACWSEnemyBase` | `ACharacter` | 기본 적 체력, 이동속도, 근접 공격, 피격/사망 애니메이션과 사망 Niagara | 구현 |
| `ACWSFastEnemy` | `ACWSEnemyBase` | 저체력·고속·짧은 공격 간격의 측면 압박 적 | 구현 |
| `ACWSTankEnemy` | `ACWSEnemyBase` | 고체력·저속·고데미지의 전면 압박 적 | 구현 |
| `ACWSEnemyAIController` | `AAIController` | 플레이어 추적 및 공격 거리 제어 | 구현 |
| `ACWSSpawnPoint` | `AActor` | 방향별 스폰 위치 | 구현 |
| `ACWSWaveManager` | `AActor` | 라운드 진행, 적 스폰, 사망/클리어 판정 | 구현 |
| `ACWSGameMode` | `AGameModeBase` | Pawn/HUD 지정, 게임 오버·클리어·레벨 재시작, 런타임 스모크 테스트 | 구현 |
| `ACWSHUD` | `AHUD` | 조준점, 체력, 탄창/예비 탄약, 재장전 상태, 라운드, 남은 적 표시 | 구현 |
| `ACWSBossEnemy` | `ACWSEnemyBase` | 3단계 페이즈, Ground Slam, 넉백 Shockwave | 구현 |
| `ACWSSupplyPickup` | `AActor` | 오버랩 수집, 체력 +40 또는 예비 탄약 +30, 회전/부유 표시 | 구현 |

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
- CurrentPhase
- PhaseStartedTime

### 함수

- StartRound(int32 RoundNumber)
- StartWaveSystem()
- StopWaveSystem()
- GetRemainingEnemyCount()
- GetWavePhase()
- GetPhaseTimeRemaining()
- GetPhaseElapsedTime()
- HandleSpawnedEnemyDeath(AActor* DeadActor)
- HandleSpawnedEnemyDestroyed(AActor* DestroyedActor)

### 현재 구현

- `ACWSWaveManager`가 Round 1~5 기본 데이터를 소유한다.
- 방향 그룹을 라운드 로빈 순서로 큐에 넣어 한 방향에 스폰이 몰리지 않게 한다.
- 생성된 적의 `OnDeath` 이벤트로 즉시 남은 적을 갱신하고 `OnDestroyed`를 안전망으로 유지한다.
- `ECWSWavePhase`가 `Idle`, `Preparing`, `Active`, `RoundCleared`, `Completed`, `Stopped` 상태를 표현한다.
- `OnWavePhaseChanged`, `OnRoundStarted`, `OnRemainingEnemyCountChanged`, `OnRoundCleared`, `OnAllRoundsCompleted` Blueprint delegate를 제공한다.
- HUD가 타이머를 직접 추측하지 않도록 현재 페이즈의 남은 시간과 경과 시간 getter를 제공한다.
- `ECWSEnemyType`과 방향별 SpawnGroup을 이용해 `ACWSEnemyBase`, `ACWSFastEnemy`, `ACWSTankEnemy`, `ACWSBossEnemy`를 선택한다.
- 저장된 기존 라운드 데이터에도 `bUseDefaultArchetypeComposition` 매핑을 적용해 맵 에셋 재저장 없이 기본 조합을 유지한다.
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

`ACWSEnemyBase`는 체력 60, 이동속도 350, 공격력 10의 Normal 적이며 근접 공격 거리/데미지/간격을 제공한다. `ACWSFastEnemy`는 체력 35, 속도 520, 공격력 8, 공격 간격 0.65초이고, `ACWSTankEnemy`는 체력 180, 속도 230, 공격력 18, 공격 간격 1.4초다. 세 타입 모두 `ACWSEnemyAIController`가 플레이어를 NavMesh로 추적하고 공격 거리 안에서 `ApplyDamage`를 호출한다. 비치명타에는 additive 피격 애니메이션을 재생하고, 사망하면 이동과 충돌을 끈 뒤 사망 애니메이션과 확대된 `NS_Damage` Niagara를 재생하며 웨이브 매니저에 `OnDeath`를 전달한다.

`UCWSHitscanWeaponComponent`는 Visibility 라인트레이스 충돌 지점에 `NS_Damage` Niagara를 생성한 뒤 포인트 데미지를 적용한다. `-nullrhi` 스모크에서는 유효한 이펙트 생성 경로 실행을 카운터로 검사하고, 렌더링 결과는 별도 오프스크린 캡처로 확인한다.

## 9. 전투 흐름 런타임 검증

- `run_build_playable_round_one.ps1 -InspectOnly`: PlayerStart, NavMesh Bounds, GameMode, 적 클래스와 Map Check 검사
- `run_build_wave_spawning.ps1 -InspectOnly`: 9개 스폰 지점, Normal/Fast/Tank/Boss 클래스 연결, `8 / 16 / 24 / 34 / 15` 라운드 수량 검사
- `run_round_one_smoke.ps1`: 실제 `TryFire()` 히트스캔 피격/사망, 피격/사망 애니메이션과 Niagara 생성 경로, 1.2초 재장전과 예비 탄약 소비, 탄약/체력 보급 수집, 적 NavMesh 이동, Round 1 클리어, `Preparing → Active → RoundCleared` 페이즈, 플레이어 피격 사망, 웨이브 정지, 현재 레벨 재시작 검사
- `run_round_one_smoke.ps1 -AllRounds`: 실제 게임 월드에서 Round 1~5의 97개 스폰, 전투 피드백, Fast/Tank 클래스와 능력치, Round 1~4 보급 생성, 각 라운드 클리어, 라운드 공지 페이즈와 최종 `Completed` 전환 검사
- `run_hud_screenshot.ps1`: Round 1 `Preparing` 상태를 1280×720 오프스크린으로 렌더링해 중앙 카운트다운 HUD 스크린샷 생성 검사
- `run_combat_feedback_screenshot.ps1`: 오프스크린 게임 월드에서 피격 애니메이션과 Niagara를 예열한 뒤 사망 포즈와 사망 Niagara가 함께 보이는 1280×720 스크린샷 생성 검사
- 전체 라운드 검증은 전용 Boss 클래스, 체력 1200, 최종 페이즈 전환, Ground Slam과 Shockwave 피해/넉백 경로도 함께 검사한다.
- Warm DDC 직렬화 오류가 감지되면 스모크 러너가 격리된 Cold DDC로 한 번 자동 재시도한다.
- `run_repair_combat_input.ps1 -InspectOnly`: `IMC_Combat`의 액션이 없는 손상 매핑 검사

적 Capsule은 `ECC_Visibility`를 차단하므로 플레이어 무기의 Visibility 채널 라인트레이스가 실제 적에게 도달한다. 플레이어 사망 시 `ACWSGameMode`가 `ACWSWaveManager::StopWaveSystem()`을 호출하고 HUD에 게임 오버와 Enter 재시작 안내를 표시한다.

무기는 60발 탄창과 시작 예비 탄약 90발(최대 120발)을 사용한다. 재장전은 1.2초 동안 발사를 잠그고 완료 시 필요한 수량만 예비 탄약에서 탄창으로 옮긴다. `ACWSGameMode`는 Round 1~4 클리어 때 홀수 라운드에는 탄약, 짝수 라운드에는 체력 보급을 플레이어 전방에 생성한다.

## 10. 확장 가능 구조

나중에 아래 기능을 추가하기 쉽도록 만든다.

- 적 타입 추가
- 무기 타입 추가
- 난이도 배율
- 보스 패턴 추가
- 협동 멀티플레이
- 미니맵/레이더
