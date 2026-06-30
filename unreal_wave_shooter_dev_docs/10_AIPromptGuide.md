# 10. AI Prompt Guide

AI에게 C++ 소스 작성을 요청할 때 사용할 수 있는 프롬프트 모음입니다.

## 1. HealthComponent 작성 요청

```text
Unreal Engine 5 C++ 프로젝트에서 사용할 UCWSHealthComponent를 작성해줘.
요구사항:
- MaxHealth, CurrentHealth, bIsDead 변수
- ApplyHealthChange(float Delta) 함수
- OnHealthChanged, OnDeath Dynamic Multicast Delegate
- BlueprintCallable / BlueprintAssignable 적용
- ActorComponent 기반
- 플레이어와 적 모두 사용할 수 있게 범용적으로 작성
- .h와 .cpp를 분리해서 제공
```

## 2. Spawn Direction Enum 작성 요청

```text
아래 방향 구조에 맞는 Unreal Engine C++ UENUM을 작성해줘.
방향:
North, South, East, West, NorthEast, NorthWest, SouthEast, SouthWest, Center
BlueprintType으로 만들고 DisplayName은 한국어로 북, 남, 동, 서, 북동, 북서, 남동, 남서, 중앙으로 설정해줘.
```

## 3. WaveManager 작성 요청

```text
Unreal Engine 5 C++에서 ACWSWaveManager 클래스를 작성해줘.
게임은 중앙 거점 웨이브 슈터이고 라운드 구성은 다음과 같아.
1R: 북, 남
2R: 동, 서, 남, 북
3R: 동, 서, 남, 북 + 북동, 남서
4R: 동, 서, 남, 북 + 북동, 북서, 남동, 남서
5R: 중앙 보스 출현

요구사항:
- RoundDataTable 기반으로 스폰
- ACWSSpawnPoint 배열 관리
- CurrentRound, RemainingEnemyCount 관리
- StartRound, EndRound, HandleEnemyDeath 함수
- Blueprint 이벤트로 라운드 시작/종료 알림
- .h와 .cpp를 분리해서 작성
```

## 4. EnemyBase 작성 요청

```text
Unreal Engine 5 C++에서 ACWSEnemyBase 클래스를 작성해줘.
요구사항:
- ACharacter 상속
- UCWSHealthComponent 사용
- EnemyType, AttackDamage, AttackRange, ScoreValue 변수
- BeginPlay에서 HealthComponent 이벤트 바인딩
- Die 함수에서 WaveManager에 사망 보고
- Blueprint에서 Mesh/Animation을 붙일 수 있게 구성
```

## 5. DataTable Struct 작성 요청

```text
Unreal Engine 5 C++에서 웨이브 슈터용 DataTable Struct를 작성해줘.
필요 Struct:
- FCWSEnemyDataRow : FTableRowBase
- FCWSSpawnGroup
- FCWSRoundDataRow : FTableRowBase

필드:
EnemyData는 EnemyType, EnemyClass, MaxHealth, MoveSpeed, AttackDamage, ScoreValue.
SpawnGroup은 Direction, EnemyType, Count, SpawnInterval.
RoundData는 RoundNumber, SpawnGroups 배열, PreRoundDelay, PostRoundDelay, bIsBossRound.
```

## 6. UI 연동 요청

```text
Unreal Engine 5 C++에서 WBP_HUD와 연동하기 위한 이벤트 구조를 설계해줘.
WaveManager 또는 GameState에서 다음 정보를 UI로 전달하고 싶어:
- 현재 라운드
- 남은 적 수
- 플레이어 체력 퍼센트
- 보스 체력 퍼센트
- 라운드 시작/클리어/게임오버 이벤트

C++ Delegate와 Blueprint 바인딩이 가능한 방식으로 설계해줘.
```

## 7. AI에게 주의시킬 내용

코드 요청 시 아래 내용을 함께 전달하면 좋다.

```text
주의:
- Unreal Engine 5.x 기준으로 작성
- 헤더와 CPP를 분리
- 불필요하게 복잡한 시스템은 피함
- Blueprint에서 조정 가능한 변수는 UPROPERTY로 노출
- 포인터 null 체크 포함
- 컴파일 가능한 형태로 작성
- 클래스명 접두사는 CWS 사용
```
