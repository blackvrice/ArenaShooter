# ArenaShooter

Unreal Engine 5.6과 C++로 만든 3인칭 아레나 웨이브 슈터입니다. 플레이어는 중앙 거점을 지키며 8방향에서 진입하는 Normal/Fast/Tank 적을 상대하고, 5라운드에서 3단계 패턴을 가진 보스를 처치합니다.

`master`에는 5라운드 전체 전투 흐름, Windows Shipping 패키징, 자동 스모크 검증까지 통합된 플레이 가능한 출시 후보가 반영되어 있습니다.

## 핵심 구현

- Round 1~5 총 97기와 방향별 스폰 구성
- Normal/Fast/Tank/Boss 능력치, 색상 마커와 발밑 밴드
- 히트스캔 사격, 60발 탄창, 1.2초 재장전, 예비 탄약과 보급
- 숄더 카메라와 마우스 시점 회전, 8방향 라이플 이동, C++ 무기 반동
- 피격/사망 애니메이션, 네이티브 발광 버스트 VFX, 런타임 합성 전투 SFX
- 준비/전투/클리어/완료 HUD와 게임 오버·재시작 흐름
- 중앙 링, 충돌·내비게이션 엄폐물 8개, 방향 게이트 비콘 8개
- 실제 사격 404발과 70% 명중률 기준 탄약 경제 자동 검증

## 실행

- Unreal Engine 5.6
- Visual Studio 2022 C++ 게임 개발 도구
- Windows 10/11

`ArenaShooter.uproject`를 열고 `/Game/Variant_Combat/Lvl_Combat` 맵에서 Play를 실행합니다. 에디터가 고부하 상태에서 불안정한 PC에서는 다음 저부하 실행 스크립트를 사용할 수 있습니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_stable_editor.ps1
```

이 스크립트는 기본적으로 DirectX 11, 축소된 스레드 사용량과 제한된 CPU affinity를 적용합니다. 필요하면 `-DirectX12` 또는 격리 DDC를 사용하는 `-ColdDdc` 옵션을 추가할 수 있습니다.

## 조작

| 입력 | 동작 |
|---|---|
| WASD | 이동 |
| 마우스 | 시점/조준 |
| 마우스 왼쪽 | 발사 |
| 마우스 오른쪽 | 재장전 |
| Enter | 게임 오버/클리어 후 현재 레벨 재시작 |

## 검증

```powershell
# Round 1과 Round 1~5 실제 게임 흐름
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1 -AllRounds

# 실제 히트스캔 404발 탄약 밸런스
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_balance_combat_test.ps1

# HUD, 피격/사망, 적 공격, 타입별 시각 구분 캡처
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_hud_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_combat_feedback_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_attack_feedback_screenshot.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_visual_polish_screenshot.ps1
```

스크립트는 `Saved/Screenshots`에 1280×720 이미지를 만들고 성공 마커와 파일 생성 여부를 함께 검사합니다. Warm DDC 손상 시 누적 재시도와 격리 Cold DDC 경로를 사용합니다.

## 리소스 안정성

Normal/Fast/Tank의 플레이용 메시와 애니메이션은 `Content/CWSResources/Enemies`에 독립 리소스로 구성되어 있습니다. 현재 Boss는 손상된 전용 파생 데이터의 재로딩을 방지하기 위해 크기를 확대한 Normal 메시 폴백을 사용하며, 보스 전용 체력·3단계 공격 패턴·HUD 마커는 그대로 유지됩니다.

라이플 발사는 Additive 애니메이션을 전신 단독 재생하지 않고 현재 이동 자세를 유지한 채 부착된 무기에 짧은 반동을 적용합니다. 이 방식으로 사격 시 캐릭터가 다른 자세로 바뀌는 문제를 방지합니다.

## Windows Shipping

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

한글이 포함된 원본 경로의 UnrealBuildTool 문제를 피하기 위해 현재 Git `HEAD`를 임시 ASCII worktree에 체크아웃합니다. 스크립트는 Win64 Shipping Build/Cook/Stage/Pak/Archive 후 실제 패키지 실행 파일에서 Round 1~5 전체 스모크를 수행합니다. 미커밋 맵·에디터 변경은 패키지에 포함되지 않습니다.

최신 검증 출시 후보와 포트폴리오용 소개·촬영 흐름은 [빌드 문서](unreal_wave_shooter_dev_docs/11_BuildAndPackaging.md)와 [포트폴리오 문서](unreal_wave_shooter_dev_docs/13_PortfolioAndRelease.md)에 정리되어 있습니다.

전체 설계와 순차 개발 기록은 [개발 문서 인덱스](unreal_wave_shooter_dev_docs/00_README.md)를 참고하세요.
