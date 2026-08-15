# 07. UI And Game Flow

## 1. HUD 구성

| UI 요소 | 표시 내용 |
|---|---|
| Health Bar | 플레이어 체력 |
| Ammo Counter | 현재 탄약 / 최대 탄약 |
| Round Text | 현재 라운드 |
| Enemy Counter | 남은 적 수 |
| Direction Warning | 활성 스폰 방향 경고 |
| Boss Health Bar | 5라운드 보스 체력 |
| Score | 점수 |

## 2. 화면 흐름

```text
Main Menu
→ Game Start
→ Round 1
→ Round Clear
→ Round 2
→ Round Clear
→ Round 3
→ Round Clear
→ Round 4
→ Round Clear
→ Round 5 Boss
→ Game Clear
```

## 3. 라운드 시작 UI

라운드 시작 전에 3초 카운트다운을 보여준다.

예시:

```text
Round 3
동 / 서 / 남 / 북 / 북동 / 남서 방향 경계
3... 2... 1...
```

## 4. 경고 표시

새 방향이 열리는 라운드에서는 화면에 경고를 준다.

| 라운드 | 경고 |
|---|---|
| 1R | 북/남에서 적 접근 |
| 2R | 동/서 스폰 추가 |
| 3R | 대각선 스폰 일부 개방 |
| 4R | 모든 방향 개방 |
| 5R | 중앙 보스 출현 |

## 5. GameState 변수

| 변수 | 설명 |
|---|---|
| CurrentRound | 현재 라운드 |
| RemainingEnemyCount | 남은 적 수 |
| Score | 점수 |
| bIsBossRound | 보스 라운드 여부 |
| bGameOver | 게임 종료 여부 |

## 6. UI 업데이트 방식

WaveManager 또는 GameState에서 이벤트를 브로드캐스트한다.

```text
OnRoundStarted
OnEnemyCountChanged
OnRoundCleared
OnBossSpawned
OnPlayerDead
OnGameCleared
```

Widget은 해당 이벤트를 받아 텍스트와 게이지를 갱신한다.

## 7. 현재 구현 상태

- `ACWSHUD` Canvas HUD가 체력, 탄약, 라운드, 남은 적 수를 표시한다.
- 플레이어 체력이 0이 되면 `ACWSGameMode`가 게임 오버 상태를 설정하고 진행 중인 웨이브 타이머를 중단한다.
- Round 5까지 모두 클리어하면 게임 클리어 상태를 설정한다.
- Round 5 Boss가 살아 있는 동안 화면 상단에 체력바, 현재 페이즈, 최근 패턴을 표시한다.
- 게임 오버/클리어 상태에서는 `PRESS ENTER TO RESTART`를 표시하고 Enter 입력으로 현재 `Lvl_Combat`를 다시 연다.
- UMG 위젯, 방향 경고, 점수, Boss 패턴 사전 경고는 후속 UI 작업이다.
