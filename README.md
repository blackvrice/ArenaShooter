# ArenaShooter

Unreal Engine 5.6과 C++로 구현한 3인칭 Arena Wave Shooter입니다. 중앙 전장을 지키며 서로 다른 역할의 적을 상대하고, 5라운드의 보스전을 돌파하는 플레이 가능한 수직 슬라이스입니다.

**Gameplay GIF:** To be added

**Gameplay Video:** To be added

| 개발 | Engine | Core | 구성 | 검증/배포 |
|---|---|---|---|---|
| 개인 프로젝트 / 1인 개발 | Unreal Engine 5.6 | C++ | 5 Rounds · 4 Enemy Types · Boss Battle | Automated Smoke Tests · Windows Shipping |

## Overview

Title 화면에서 직접 전투를 시작한 뒤 8방향 스폰에서 진입하는 Normal, Fast, Tank를 히트스캔 라이플로 저지하고, 보급으로 전투 자원을 관리해 3단계 Boss를 처치하는 웨이브 슈터입니다. Title부터 Final Clear, Game Over, Restart까지 끊김 없는 게임 루프로 완성했습니다.

## Core Gameplay

`Title → Combat → Wave → Supply → Boss → Clear / Game Over → Restart`

- Normal은 기본 압박, Fast는 기동 압박, Tank는 높은 체력과 공격력, Boss는 Ground Slam과 Shockwave로 역할이 구분됩니다.
- 무기는 실제 Visibility 라인트레이스, 60발 탄창, 1.2초 재장전, 예비 탄약을 사용합니다.
- HUD는 체력·탄약·재장전·라운드·남은 적·Boss 페이즈와 종료 상태를 표시합니다.

## Technical Highlights

1. **Unreal C++ gameplay loop** — Player, Weapon, Enemy AI, Wave 상태 머신, Supply, Boss, HUD를 이벤트 기반으로 연결했습니다.
2. **실제 Hitscan 탄약 경제 검증** — 계산만 비교하지 않고 `TryFire()`의 라인트레이스·데미지·발사 간격·재장전·보급 경로로 97기를 처치합니다.
3. **자동 회귀검증** — Round 1/전체 라운드 Smoke, 밸런스, Title/HUD/전투/공격/비주얼 캡처를 테스트 전용 Runner로 분리했습니다.
4. **Windows Shipping 실행 검증** — Build/Cook/Stage/Pak/IoStore/Archive 뒤 패키지 실행 파일에서 Round 1~5를 다시 검증합니다.

## Architecture

`ACWSGameMode`는 Title 시작 상태, Player/Wave 연결, Round Clear, Game Over, Restart 같은 런타임 흐름만 담당합니다. 명령행 검증은 `FCWSGameplayTestCoordinator`가 감지하고 아래 Runner에 위임합니다.

| Runner | 책임 |
|---|---|
| `FCWSCombatSmokeRunner` | Round 1, Round 1~5, 사망/재시작, 전투 피드백, 보급, 적 타입, Boss 패턴 |
| `FCWSBalanceTestRunner` | 실제 Hitscan 404발과 70% 명중률 기준 탄약 경제 |
| `FCWSScreenshotTestRunner` | Title→Round 1, HUD, 피격/사망, 적 공격, 아레나·4종 적 시각 QA와 PNG 무결성 |

Production gameplay 상태를 테스트에 복제하지 않고 실제 Weapon, Enemy, Wave 객체를 사용합니다. 상세 책임 비교는 [C++ Architecture](unreal_wave_shooter_dev_docs/03_CPPArchitecture.md)에 정리했습니다.

## Boss Presentation

전용 Boss 메시의 불안정한 파생 데이터를 다시 사용하지 않고 검증된 Normal 메시를 폴백으로 유지했습니다. 대신 1.65배 실루엣, 전용 Crown, 확대 Aura Ring, 강한 Point Light를 네이티브 컴포넌트로 구성하고 `보라 → 주황 → 적색`으로 페이즈에 따라 바뀌게 해 일반 적의 단순 확대처럼 보이지 않도록 했습니다. 체력 1200, 3 Phase, Ground Slam, Shockwave 규칙은 그대로 유지됩니다.

## Problem Solving

### Ammo Economy

- **문제:** 기존 탄약 210발로는 적 97기의 체력과 무기 데미지 기준 최소 404발이 필요한 5라운드를 완주할 수 없었습니다.
- **분석/해결:** 라운드별 필요 명중 수를 `24 / 40 / 104 / 132 / 104`로 계산하고 시작 예비 탄약과 라운드 보급을 조정했습니다.
- **검증:** 실제 `TryFire()`·Hitscan·Reload·Supply 경로로 404발을 모두 적중시켰고, 70% 명중률 기준 필요 578발보다 많은 총 600발을 확보했습니다.

### Unreal Shipping Resource Stability

- **문제:** Editor에서 동작하던 Niagara 효과가 Shipping 기본 맵 로드 중 `UNiagaraStatelessEmitter::Serialize`에서 접근 위반을 일으켰습니다.
- **분석/해결:** PDB와 CrashContext로 스택을 심볼화한 뒤, 피격/사망 효과를 외부 파생 데이터에 의존하지 않는 네이티브 메시·라이트 버스트로 교체했습니다.
- **검증:** Shipping Build/Cook/Pak 이후 실제 패키지 실행 파일에서 Round 1~5 스모크를 수행했습니다.

### Rifle Animation

- **문제:** Additive 라이플 애니메이션을 전신 단독 재생하면 이동 자세와 캐릭터 실루엣이 깨졌습니다.
- **해결:** 현재 이동 Pose를 유지하고 부착된 라이플에 짧은 C++ 반동을 적용했습니다.
- **검증:** 사격 입력, 실제 라인트레이스 피격, 반동 복귀를 전투 스모크와 오프스크린 캡처 경로에서 함께 확인했습니다.

## AI-assisted Development

- 개발자가 요구사항, 게임 규칙, 구조, 테스트 합격 기준을 정의했습니다.
- 생성형 AI는 C++ 초안, 반복 구현, 리팩터링 후보와 테스트 코드 작성 보조에 활용했습니다.
- 개발자가 생성 코드를 리뷰하고 실패 로그와 실제 런타임 동작을 기준으로 수정 방향을 결정했습니다.
- 테스트를 통과시키기 위해 조건을 완화하지 않고 실제 Gameplay 원인을 수정했습니다.
- 과정은 `Requirements → AI-assisted Implementation → Human Code Review → Build → Automated Test → Play Test → Shipping Verification`입니다.
- 이 프로젝트는 **Human-directed, AI-assisted, Test-verified** 개발 프로젝트입니다.

## 실행 및 조작

요구 환경은 Windows 10/11, Unreal Engine 5.6, Visual Studio 2022 C++ 게임 개발 도구입니다. `ArenaShooter.uproject`를 열고 `/Game/Variant_Combat/Lvl_Combat`에서 Play를 실행합니다.

| 입력 | 동작 |
|---|---|
| WASD | 이동 |
| 마우스 | 시점/조준 |
| 마우스 왼쪽 | 발사 |
| 마우스 오른쪽 | 재장전 |
| Enter | Title에서 게임 시작 / Game Over·Clear 후 즉시 재시작 |

고부하 환경에서는 저부하 실행 스크립트를 사용할 수 있습니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_stable_editor.ps1
```

## Automated Verification

```powershell
# Runtime combat flow
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1 -AllRounds

# Actual hitscan balance
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_balance_combat_test.ps1

# 1280×720 rendered QA
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_title_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_hud_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_combat_feedback_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_attack_feedback_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_visual_polish_screenshot.ps1
```

스크린샷 테스트는 상태 마커뿐 아니라 완전한 PNG signature와 `IEND` 청크를 검사합니다. Title 테스트는 웨이브가 시작되지 않은 대기 화면을 캡처한 뒤 Round 1 준비 상태로 전환되는 것까지 확인합니다. 생성 이미지는 `Saved/Screenshots`에서 사람이 직접 확인합니다.

## Windows Shipping

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

스크립트는 한글 경로에서 발생하는 UnrealBuildTool 문제를 피하기 위해 현재 Git `HEAD`를 임시 ASCII worktree에 체크아웃합니다. Win64 Shipping Build/Cook/Stage/Pak/IoStore/Archive 후 실제 패키지의 `ArenaShooter.exe`에서 전체 라운드 스모크를 실행합니다. 미커밋 변경은 패키지에 포함되지 않습니다.

최신 검증 결과와 배포 체크리스트는 [Build & Packaging](unreal_wave_shooter_dev_docs/11_BuildAndPackaging.md), [Portfolio & Release](unreal_wave_shooter_dev_docs/13_PortfolioAndRelease.md), [Release Checklist](unreal_wave_shooter_dev_docs/14_ReleaseChecklist.md)에 기록합니다. 변경 전후 구조, 실제 검증, 기술 부채와 면접 질문은 [Final Portfolio Report](unreal_wave_shooter_dev_docs/15_FinalPortfolioReport.md), 전체 문서는 [개발 문서 인덱스](unreal_wave_shooter_dev_docs/00_README.md)에서 확인할 수 있습니다.
