# ArenaShooter

Unreal Engine 5.6 기반 3인칭 웨이브 슈터입니다. `Lvl_Combat`에서 8방향 스폰, Normal/Fast/Tank 적, 5라운드 보스전, 히트스캔 사격, 재장전, 보급, 게임 오버와 재시작 흐름을 제공합니다.

## 검증

에디터 전투 흐름:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_round_one_smoke.ps1 -AllRounds
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Editor\run_hud_screenshot.ps1
```

`run_hud_screenshot.ps1`은 Round 1 준비 카운트다운을 오프스크린으로 렌더링하고 `Saved/Screenshots/CWSRoundAnnouncement.png`를 생성해 HUD가 실제 화면에 표시되는지 검사한다.

Windows Shipping 패키지 생성과 배포본 전체 라운드 스모크:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

프로젝트 경로에 한글이 포함되어 있어 Shipping 컴파일은 임시 ASCII Git worktree에서 수행한다. 패키지는 기본적으로 `C:\ArenaShooterPackages` 아래에 생성되며 임시 worktree는 성공 여부와 관계없이 정리된다. 스크립트는 미커밋 파일이 아닌 현재 `HEAD`를 패키징한다.

상세 설계와 현재 개발 상태는 [개발 문서](unreal_wave_shooter_dev_docs/00_README.md)를 참고한다.
