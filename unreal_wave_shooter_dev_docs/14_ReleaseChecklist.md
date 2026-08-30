# 14. Release Checklist

## Release identity

| 항목 | 값 |
|---|---|
| Tag | `v1.0.0` (기존 태그가 있으면 다음 적절한 Portfolio RC 태그 사용) |
| Release Name | `ArenaShooter v1.0.0 - NexTorial Portfolio Build` |
| Asset Name | `ArenaShooter-v1.0.0-Windows.zip` |
| Target Branch | `master` |
| Engine | Unreal Engine 5.6 |

실제 ZIP 경로·크기·SHA-256은 현재 `master` HEAD의 Shipping 검증이 끝난 뒤에만 이 문서에 확정한다. 이전 커밋의 ZIP이나 존재하지 않는 파일을 새 릴리스 자산으로 사용하지 않는다.

## Required validation

- [x] `ArenaShooterEditor Win64 Development` C++ compile
- [x] Map Check 오류 0 / 경고 0
- [x] Round 1 `CWS_ROUND_ONE_SMOKE_SUCCESS`
- [x] Round 1~5 `CWS_ALL_ROUNDS_SMOKE_SUCCESS`
- [x] 실제 Hitscan `CWS_BALANCE_COMBAT_SUCCESS`
- [x] HUD/Combat/Attack/Visual 캡처 4종과 유효 PNG 확인
- [x] 캡처 4종 직접 시각 검수
- [ ] Shipping Build/Cook/Stage/Pak/IoStore/Archive
- [ ] 패키지 EXE의 `CWS_ALL_ROUNDS_SMOKE_SUCCESS`
- [ ] 패키징 스크립트의 `CWS_PACKAGE_VERIFICATION_SUCCESS`
- [ ] PDB 제외 배포 ZIP 생성과 SHA-256 기록
- [ ] 깨진 README 링크와 존재하지 않는 자산 표기 없음

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
- GameMode에서 분리한 Combat / Balance / Screenshot QA Runner
- 실제 사격 404발과 70% 명중률 기준 탄약 경제 자동 검증
- Round 1, Round 1~5, 사망/재시작, 캡처 4종 회귀검증
- Windows Shipping 패키지 실행 파일 전체 라운드 검증

## Release asset record

현재 포트폴리오 변경분의 새 Shipping 실행 검증 전 상태: **Pending**

| 항목 | 검증 후 기록 |
|---|---|
| Source commit | Pending |
| Archive directory | Pending |
| Verified executable | Pending |
| ZIP path | Pending |
| ZIP size | Pending |
| SHA-256 | Pending |
| Packaged smoke marker | Pending |

## Manual presentation handoff

- Gameplay GIF와 Gameplay Video는 실제 플레이를 녹화한 뒤 README의 TODO를 교체한다.
- 영상에는 이동/조준, Fire+Hit, Reload, Fast/Tank, Supply, Boss, Ground Slam/Shockwave, Final Clear를 포함한다.
- 배포 전 깨끗한 PC 또는 별도 Windows 계정에서 ZIP 압축 해제 후 실행하는 최종 수동 QA를 권장한다.
