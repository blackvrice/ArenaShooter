# ArenaShooter

Unreal Engine 5.6과 C++로 Title부터 5라운드 Boss전까지 완성하고, 자동 회귀검증과 Windows Shipping 배포본 검증까지 수행한 3인칭 Wave Shooter입니다.

**Gameplay GIF:** To be added

**Gameplay Video:** To be added

| 개발 | Engine | Language | 핵심 | 검증 / 배포 |
|---|---|---|---|---|
| 개인 프로젝트 / 1인 개발 | Unreal Engine 5.6 | C++ | Hitscan Combat · Wave/Boss · Game Flow | Current All-Rounds Smoke · Verified Windows Shipping Record |

## Overview

플레이어는 중앙 Arena를 지키며 8방향에서 진입하는 Normal, Fast, Tank를 Hitscan Rifle로 처치하고, 보급으로 체력과 탄약을 관리해 5라운드 Boss를 쓰러뜨립니다. `Title → Combat → Final Clear / Game Over → Restart`가 실제 플레이 가능한 하나의 루프로 연결되며, 구현 이후 실제 Gameplay 경로와 패키지 실행 파일까지 검증하는 것을 기술 목표로 삼았습니다.

## Core Gameplay

`Title → Combat → Wave → Supply → Boss → Clear / Game Over → Restart`

- Round 1~4는 적 조합과 수가 증가하고, Round 5는 3 Phase의 Boss가 Ground Slam과 Shockwave를 사용합니다.
- Rifle은 Visibility Line Trace, 발사 간격, 60발 Magazine, 시간 기반 Reload와 Reserve Ammo를 사용합니다.
- HUD는 HP, Ammo, Reload, Round, Remaining Enemy, Boss Phase와 종료 상태를 표시합니다.

## Technical Highlights

1. **Unreal C++ Gameplay Architecture** — Player/Weapon, Enemy AI, Wave, Boss, Supply, GameMode와 HUD를 이벤트 중심으로 연결하고, 테스트 orchestration은 Private Coordinator/Runner로 분리했습니다.
2. **Actual Hitscan Balance Verification** — 수식만 비교하지 않고 실제 `TryFire()`의 camera ray, line trace, fire interval, damage, death, reload와 supply 경로로 97기를 처치합니다.
3. **Automated Regression Tests** — Round 1/1~5, Game Over/Restart, 적 타입·Boss 패턴, 전투 feedback, 탄약 경제와 Screenshot 파일 무결성을 명령행 Runner로 검증합니다.
4. **Windows Shipping Verification** — ASCII worktree에서 Build/Cook/Stage/Pak/IoStore/Archive를 수행하고, Packaged EXE와 ZIP 압축 해제본에서 Round 1~5를 실행했습니다.

## Architecture

```text
Player / Hitscan Weapon
          │
          ▼
      CWSGameMode
          │
          ├─ WaveManager → SpawnPoint → Enemy / Boss
          ├─ Supply / Game Flow → HUD / Feedback
          └─ GameplayTestCoordinator
                    ├─ CombatSmokeRunner
                    ├─ BalanceTestRunner
                    └─ ScreenshotTestRunner
```

`ACWSGameMode`는 Title 시작, Player/Wave 연결, Round Clear, Game Over와 Restart만 담당합니다. 명령행 옵션 선택과 테스트 수명은 `FCWSGameplayTestCoordinator`가 관리하고, 각 Runner는 Production Weapon·Enemy·Wave 객체를 그대로 사용합니다. 상세 책임과 클래스 관계는 [C++ Architecture](unreal_wave_shooter_dev_docs/03_CPPArchitecture.md)에서 확인할 수 있습니다.

## Problem Solving

### Ammo Economy

**Problem** 초기 총 탄약 210발로는 5라운드의 적 97기를 처치하는 데 필요한 최소 404발을 충족할 수 없었습니다.

**Cause** 라운드별 HP와 weapon damage를 합산한 탄약 요구량이 시작 Reserve와 보급 설계에 반영되지 않았습니다.

**Solution** 라운드별 필요 명중 수 `24 / 40 / 104 / 132 / 104`를 기준으로 시작 탄약과 Round Supply를 재조정했습니다.

**Verification** 실제 `TryFire()` 경로에서 404 hit / 0 miss를 기록했고, 70% 명중률 기준 필요 578발보다 많은 총 600발을 제공하는지 Balance Test로 확인했습니다.

### Editor 성공 후 Shipping 시작 Crash

**Problem** Editor에서 동작하던 Niagara feedback이 Shipping 기본 맵 로드 중 `UNiagaraStatelessEmitter::Serialize`에서 접근 위반을 일으켰습니다.

**Cause** Editor cache에서는 드러나지 않던 불안정한 cooked derived data가 패키지 로드 경로에 포함됐습니다.

**Solution** CrashContext와 PDB stack을 확인한 뒤 피격·사망 효과를 외부 파생 데이터에 의존하지 않는 native mesh/light burst로 교체했습니다.

**Verification** Win64 Shipping Build/Cook/Pak을 완료하고 실제 Packaged EXE에서 Round 1~5 Smoke를 통과했습니다.

### Additive Rifle Animation

**Problem** Additive fire clip을 full-body standalone animation으로 재생해 이동 Pose와 캐릭터 실루엣이 깨졌습니다.

**Cause** Additive clip에 필요한 base pose 합성 없이 `PlayAnimation()`으로 직접 재생했습니다.

**Solution** Locomotion Pose는 유지하고 부착된 rifle mesh에 짧은 C++ recoil transform만 적용했습니다.

**Verification** C++ 빌드와 실제 사격·line trace 경로는 통과했습니다. 전용 AnimBP additive slot 적용은 후속 개선으로 남겼습니다.

## Testing / Verification

현재 `master`(`093aca6`)에서 2026-08-31 **Round 1~5 All-Rounds Smoke 재통과**를 확인했습니다. 검증된 Shipping ZIP 기록은 `f3bf9c7` 기준이며, 이후 최신 커밋에는 에디터 설정과 맵 External Actor 복원이 포함됩니다.

| 검증 범위 | 기록된 결과 |
|---|---|
| Spawn / Wave | 9 Spawn Points · `8 / 16 / 24 / 34 / 15` · 총 97기 |
| Balance | 실제 Hitscan 404 hit · 0 miss · 가용 600 / 70% 기준 필요 578 |
| Screenshot QA | Title · HUD · Combat · Attack · Visual PNG signature / `IEND` 검사 |
| Shipping | Build/Cook/Stage/Pak/IoStore/Archive · Packaged EXE All Rounds |
| Release ZIP | 71 entries · PDB 0 · 실제 파일 28개 hash 일치 · 압축 해제본 실행 |

대표 명령만 아래에 남기고 전체 옵션과 성공 marker는 상세 문서로 분리했습니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1 -AllRounds
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_balance_combat_test.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

## AI-assisted Development

생성형 AI는 C++ 구현 초안, 반복 코드, 리팩터링 후보, 테스트 작성과 실패 로그 분석을 보조했습니다. 개발자가 요구사항, 게임 규칙, 구조와 합격 기준을 먼저 정의하고 생성 코드를 직접 검토·수정했습니다. 자동 테스트 조건을 약화하지 않고 실제 runtime/packaged 경로에서 최종 판단했습니다.

## Technical Documentation

- [C++ Architecture](unreal_wave_shooter_dev_docs/03_CPPArchitecture.md) — Runtime과 Test 책임 분리
- [Build & Packaging](unreal_wave_shooter_dev_docs/11_BuildAndPackaging.md) — Shipping, ASCII worktree, DDC 복구
- [Completion Log](unreal_wave_shooter_dev_docs/12_CompletionProgressLog.md) — 단계별 구현과 검증 기록
- [Portfolio & Release](unreal_wave_shooter_dev_docs/13_PortfolioAndRelease.md) — 촬영·배포 후보 정보
- [Release Checklist](unreal_wave_shooter_dev_docs/14_ReleaseChecklist.md) — ZIP과 실행 검증
- [Final Portfolio Report](unreal_wave_shooter_dev_docs/15_FinalPortfolioReport.md) — 변경 전후, 기술 부채, 면접 질문

## Build & Run

요구 환경은 Windows 10/11, Unreal Engine 5.6, Visual Studio 2022의 C++ Game Development workload입니다. `ArenaShooter.uproject`를 열고 `/Game/Variant_Combat/Lvl_Combat`에서 Play합니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_stable_editor.ps1
```

| 입력 | 동작 |
|---|---|
| WASD | 이동 |
| Mouse | 시점 / 조준 |
| Left Click | 발사 |
| Right Click | 재장전 |
| Enter | Title 시작 / 종료 후 Restart |

## Current Scope

- Gameplay GIF와 60~90초 실제 사람 조작 영상은 아직 없습니다.
- 검증 ZIP은 기록돼 있지만 GitHub Release에 첨부되지 않았고, clean PC QA는 남아 있습니다.
- 최신 `master`의 에디터·맵 복원 뒤 Editor All-Rounds Smoke는 통과했지만, 해당 HEAD의 Shipping 재패키징은 아직 실행하지 않았습니다.
- Boss는 native component로 구분했으며 전용 제작 mesh/animation은 사용하지 않습니다.
