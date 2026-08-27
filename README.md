# ArenaShooter

Unreal Engine 5.6과 C++로 만든 3인칭 아레나 웨이브 슈터입니다. 플레이어는 중앙 거점을 지키며 8방향에서 진입하는 Normal/Fast/Tank 적을 상대하고, 5라운드에서 3단계 패턴을 가진 보스를 처치합니다.

## 핵심 구현

- Round 1~5 총 97기와 방향별 스폰 구성
- Normal/Fast/Tank/Boss 능력치, 색상 마커와 발밑 밴드
- 히트스캔 사격, 60발 탄창, 1.2초 재장전, 예비 탄약과 보급
- 피격/사망 애니메이션, 네이티브 발광 버스트 VFX, 런타임 합성 전투 SFX
- 준비/전투/클리어/완료 HUD와 게임 오버·재시작 흐름
- 중앙 링, 충돌·내비게이션 엄폐물 8개, 방향 게이트 비콘 8개
- 실제 사격 404발과 70% 명중률 기준 탄약 경제 자동 검증

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

## Windows Shipping

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

한글이 포함된 원본 경로의 UnrealBuildTool 문제를 피하기 위해 현재 Git `HEAD`를 임시 ASCII worktree에 체크아웃합니다. 스크립트는 Win64 Shipping Build/Cook/Stage/Pak/Archive 후 실제 패키지 실행 파일에서 Round 1~5 전체 스모크를 수행합니다. 미커밋 맵·에디터 변경은 패키지에 포함되지 않습니다.

최신 검증 출시 후보와 포트폴리오용 소개·촬영 흐름은 [빌드 문서](unreal_wave_shooter_dev_docs/11_BuildAndPackaging.md)와 [포트폴리오 문서](unreal_wave_shooter_dev_docs/13_PortfolioAndRelease.md)에 정리되어 있습니다.

전체 설계와 순차 개발 기록은 [개발 문서 인덱스](unreal_wave_shooter_dev_docs/00_README.md)를 참고하세요.
