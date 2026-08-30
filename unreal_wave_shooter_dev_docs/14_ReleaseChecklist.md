# 14. Release Checklist

## Release identity

| 항목 | 값 |
|---|---|
| Tag | `v1.0.0` (기존 태그가 있으면 다음 적절한 Portfolio RC 태그 사용) |
| Release Name | `ArenaShooter v1.0.0 - NexTorial Portfolio Build` |
| Asset Name | `ArenaShooter-v1.0.0-Windows.zip` |
| Target Branch | `master` |
| Engine | Unreal Engine 5.6 |

아래 자산은 Title을 포함한 포트폴리오 코드 커밋 `500d2d9`을 직접 Shipping 패키징하고 실행 검증한 결과다. 이후 패키징 스크립트·문서 전용 커밋이 추가되어도 게임 바이너리 자산의 소스 커밋은 `500d2d9`로 유지한다.

## Required validation

- [x] `ArenaShooterEditor Win64 Development` C++ compile
- [x] Map Check 오류 0 / 경고 0
- [x] Round 1 `CWS_ROUND_ONE_SMOKE_SUCCESS`
- [x] Round 1~5 `CWS_ALL_ROUNDS_SMOKE_SUCCESS`
- [x] 실제 Hitscan `CWS_BALANCE_COMBAT_SUCCESS`
- [x] Title/HUD/Combat/Attack/Visual 캡처 5종과 유효 PNG 확인
- [x] 캡처 5종 직접 시각 검수
- [x] Shipping Build/Cook/Stage/Pak/IoStore/Archive
- [x] 패키지 EXE의 Round 1~5 스모크 종료 코드 0
- [x] 패키징 스크립트의 `CWS_PACKAGE_VERIFICATION_SUCCESS`
- [x] PDB 제외 배포 ZIP 생성과 SHA-256 기록
- [x] README 로컬 링크 5개 존재 확인, 존재하지 않는 미디어는 링크 대신 TODO 표기

## Packaging and verification

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

이 스크립트는 커밋된 Git `HEAD`를 ASCII 임시 worktree로 복제한다. 따라서 릴리스 대상 변경을 먼저 커밋한 뒤 실행해야 한다. 결과는 Build/Cook/Stage/Pak/IoStore/Archive뿐 아니라 패키지의 `Windows\ArenaShooter.exe`에서 Round 1~5 스모크가 종료 코드 0으로 끝났는지 확인해야 한다.

## Release notes draft

### ArenaShooter v1.0.0 - NexTorial Portfolio Build

- Unreal Engine 5.6 C++ 기반 5 Round Arena Wave Shooter
- Normal / Fast / Tank / Boss 4종 적과 3단계 Boss Phase
- 실제 Hitscan, 시간 기반 Reload, Ammo / Health Supply
- Crown, Aura Ring, Point Light와 페이즈 색 전환을 사용한 Boss 표현
- Enter로 시작하는 영상용 Title 화면과 자동 Title→Round 1 검증
- GameMode에서 분리한 Combat / Balance / Screenshot QA Runner
- 실제 사격 404발과 70% 명중률 기준 탄약 경제 자동 검증
- Round 1, Round 1~5, 사망/재시작, 캡처 5종 회귀검증
- Windows Shipping 패키지 실행 파일 전체 라운드 검증

## Release asset record

현재 포트폴리오 변경분의 Shipping 실행 검증 상태: **Verified**

| 항목 | 검증 기록 |
|---|---|
| Source commit | `500d2d9` |
| Recovered staging directory | `C:\ArenaShooterPackageWork\ArenaShooter-20260830-235653-500d2d9\Saved\StagedBuilds` |
| Verified executable | `Windows\ArenaShooter\Binaries\Win64\ArenaShooter-Win64-Shipping.exe` |
| ZIP path | `C:\ArenaShooterPackages\ArenaShooter-v1.0.0-Windows.zip` |
| ZIP size | 334,856,723 bytes, 71 entries, PDB 0 |
| ZIP SHA-256 | `14E78124DE6263E9A2D07368B70ECB19FBB1DACAA01D167BCB0D393FB2AA1CD9` |
| EXE SHA-256 | `90A1C560DB52D1E5EA35C159A915C75BC8DB6653AE6D67C07038CF7E8BDB90F9` |
| Packaged smoke marker | `CWS_PACKAGE_VERIFICATION_SUCCESS` |

## Manual presentation handoff

- Gameplay GIF와 Gameplay Video는 실제 플레이를 녹화한 뒤 README의 TODO를 교체한다.
- 영상은 Title을 3~5초 보여준 뒤 Enter 전환, 이동/조준, Fire+Hit, Reload, Fast/Tank, Supply, Boss, Ground Slam/Shockwave, Final Clear를 포함한다.
- 배포 전 깨끗한 PC 또는 별도 Windows 계정에서 ZIP 압축 해제 후 실행하는 최종 수동 QA를 권장한다.
