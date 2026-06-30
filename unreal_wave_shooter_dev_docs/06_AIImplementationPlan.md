# 06. AI Implementation Plan

## 1. 적 AI 목표

초기 버전은 복잡한 행동보다 안정적인 추적/공격을 우선한다.

```text
스폰
→ 플레이어 위치 확인
→ 중앙 방향 또는 플레이어 방향으로 이동
→ 공격 범위 진입
→ 공격
→ 피격/사망
```

## 2. 적 AI 1차 구현

C++ 또는 Blueprint에서 `AI Move To` 기반으로 구현한다.

### 기본 행동

| 상태 | 설명 |
|---|---|
| Spawn | 스폰 직후 대기 0.2~0.5초 |
| Chase | 플레이어 추적 |
| Attack | 공격 범위 안에서 공격 |
| Dead | 충돌 비활성화 후 제거 |

## 3. NavMesh 기준

- 맵 전체 바닥에 `NavMeshBoundsVolume` 배치
- 중앙 엄폐물은 NavMesh가 자연스럽게 회피하도록 배치
- 너무 좁은 통로는 적이 끼일 수 있으므로 최소 400 uu 이상 권장

## 4. Enemy 타입별 AI 차이

### Normal

- 플레이어를 직접 추적
- 근접 공격
- 가장 기본 적

### Fast

- 이동속도 높음
- 체력 낮음
- 플레이어 측면 압박용

### Tank

- 이동속도 낮음
- 체력 높음
- 중앙으로 천천히 압박

### Boss

- 5라운드 중앙 출현
- 패턴 공격
- 주기적으로 잡몹 소환 가능

## 5. 보스 패턴 후보

| 패턴 | 설명 |
|---|---|
| Ground Slam | 근거리 원형 범위 공격 |
| Radial Shot | 사방으로 투사체 발사 |
| Summon Minions | 일정 체력 이하에서 일반 적 소환 |
| Charge | 플레이어 방향으로 돌진 |
| Shield Phase | 잠시 방어 상태 |

## 6. Boss Round 설계

```text
Round 5 시작
→ 중앙 보스 스폰
→ 플레이어를 중앙 밖으로 밀어내는 패턴 사용
→ 일정 체력마다 보조 적 스폰
→ 보스 처치 시 게임 클리어
```

## 7. Behavior Tree 확장 방향

초기 완성 후 아래 구조로 확장 가능하다.

```text
Blackboard
├─ TargetActor
├─ LastKnownLocation
├─ bCanAttack
├─ bIsDead
└─ EnemyType

Behavior Tree
├─ Selector
│  ├─ Dead
│  ├─ Attack
│  ├─ Chase
│  └─ Patrol/MoveToCenter
```
