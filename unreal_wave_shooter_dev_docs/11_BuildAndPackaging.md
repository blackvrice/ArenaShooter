# 11. Build And Packaging

## 1. Windows Shipping 패키징

저장소 루트에서 다음 스크립트를 실행한다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

스크립트는 다음 단계를 한 번에 수행한다.

1. 현재 Git `HEAD`를 임시 ASCII 경로의 detached worktree로 체크아웃한다.
2. Unreal Automation Tool로 Win64 Shipping Build/Cook/Stage/Pak/Archive를 실행한다.
3. 패키지 내부의 `ArenaShooter-Win64-Shipping.exe`를 `-CWSAllRoundsSmokeTest`로 실행한다.
4. 배포본이 제한 시간 안에 종료 코드 0으로 끝나는지 확인한다.
5. 임시 worktree를 정리하고 패키지 출력은 보존한다.

기본 출력 폴더는 `C:\ArenaShooterPackages\ArenaShooter-<시각>-<커밋>`이다. 다른 ASCII 경로가 필요하면 `-WorkspaceParent`와 `-PackageOutputParent`를 지정한다.

## 2. ASCII worktree가 필요한 이유

현재 저장소 경로에는 한글 `문서`가 포함된다. UE 5.6 UnrealBuildTool이 이 경로에서 Shipping SharedPCH를 컴파일할 때 `C1083`으로 소스 파일을 찾지 못한다. Development 에디터 빌드가 성공해도 Shipping 패키징은 별도로 실패할 수 있다.

패키징 스크립트는 원본 작업 트리를 복사하거나 수정하지 않고, 커밋된 소스만 ASCII 경로에 체크아웃한다. 따라서 원본에 남아 있는 맵·에디터 미커밋 변경은 패키지에 포함되지 않는다.

## 3. 기존 패키지만 재검증

이미 생성된 Archive 폴더가 있으면 Cook 없이 실행 스모크만 다시 수행할 수 있다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1 `
    -ExistingPackageDirectory C:\ArenaShooterPackages\ArenaShooter-20260815-190000-840eeaa
```

성공 기준은 `CWS_PACKAGE_VERIFICATION_SUCCESS` 출력과 프로세스 종료 코드 0이다. Shipping에서는 일반 로그 출력이 비활성화될 수 있으므로, 게임이 전체 라운드 스모크 성공 시 호출하는 종료 코드를 최종 판정으로 사용한다.

## 4. 2026-08-15 검증 결과

- 검증 커밋: `840eeaa`
- Win64 Shipping 컴파일 성공
- Cook 520개 패키지, 오류 0 / 경고 0
- Stage/Pak/IoStore/Archive 성공
- 패키지의 실제 Shipping 실행 파일로 전체 라운드 스모크 종료 코드 0 확인
