# 11. Build And Packaging

## 1. Windows Shipping 패키징

저장소 루트에서 다음 스크립트를 실행한다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1
```

공유 Warm DDC에서 `OodleLZ_Decompress` 또는 `FLargeMemoryReader` 손상이 반복되면 캐시를 삭제하지 않고 격리된 filesystem DDC로 재시도한다. 복구 경로는 `InstalledNoZenLocalFallback`으로 검증된 Engine Pak을 재사용하고, 프로젝트 파생 데이터는 임시 worktree 내부에만 기록한다. Cook은 `-corelimit=8`, 최종 IoStore/UnrealPak은 기본 2코어로 제한해 Intel 13/14세대 CPU에서 발생할 수 있는 Oodle 셰이더 압축 불안정을 낮춘다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Build\run_package_windows.ps1 -ColdDdc
```

복구용 UnrealPak 코어 수는 필요할 때 `-RecoveryPakCoreLimit 1`처럼 1~8 범위에서 조정할 수 있다. 실패한 Cook 결과를 조사하거나 같은 worktree에서 후속 복구를 진행해야 할 때는 `-KeepWorkspace`를 함께 지정한다.

CPU 자체의 접근 위반이나 Oodle 불안정이 이어지는 특정 PC에서는 `-RecoveryAffinityMask <십진수 비트마스크>`로 AutomationTool과 빌드 자식 프로세스를 검증한 코어 집합에 제한할 수 있다. 패키지 생성이 끝나면 원래 affinity를 복원한 뒤 Shipping 스모크를 실행한다. 이 값은 CPU 토폴로지마다 다르므로 다른 PC의 값을 그대로 복사하지 않는다.

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

## 5. 2026-08-27 출시 후보

- 게임 코드 커밋: `32218bc`
- Win64 Shipping/Editor 컴파일 성공
- Cook 515개 패키지, Stage/Pak/IoStore/Archive 성공
- 실제 `ArenaShooter-Win64-Shipping.exe`에서 Round 1~5 전체 스모크 종료 코드 0
- 성공 마커: `CWS_PACKAGE_VERIFICATION_SUCCESS`
- 보존 패키지: `C:\ArenaShooterPackages\ArenaShooter-20260827-202923-32218bc`
- 배포 ZIP: `C:\ArenaShooterPackages\ArenaShooter-20260827-202923-32218bc-Windows.zip`
- ZIP 크기: 319,659,273 bytes
- ZIP SHA-256: `84A1301D83EE678834A0C6D6F38B6AF650AFDA053ADCAD81D0981E4932955453`
- Shipping 실행 파일 SHA-256: `F37CB6B634FD5AC1792DD15B8DC98A9244A269D214BC039E3A97F76044A934F7`

첫 두 출시 후보는 `UNiagaraStatelessEmitter::Serialize`에서 Shipping 전용 접근 위반이 발생해 승인하지 않았다. 피격/사망 효과를 `ACWSCombatBurstEffect` 네이티브 메시·라이트 버스트로 교체하고 Niagara 런타임 참조를 제거한 뒤 기본 맵 로드와 전체 라운드 패키지 검증을 통과했다. 원본 Archive는 PDB를 보존하고, 배포 ZIP은 PDB를 제외했다.

## 6. 2026-08-30 NexTorial 포트폴리오 빌드

- 소스 커밋: `6ce5ed6` (`refactor gameplay QA and polish boss presentation`)
- Win64 Shipping/Editor 컴파일과 UnrealHeaderTool 성공
- Cook 565개 패키지, Stage/Pak/IoStore/Archive 성공
- 실제 `ArenaShooter-Win64-Shipping.exe`에서 Round 1~5 스모크 종료 코드 0
- 성공 마커: `CWS_PACKAGE_VERIFICATION_SUCCESS: commit 6ce5ed6 was packaged and passed the all-round Shipping smoke test.`
- 보존 Archive: `C:\ArenaShooterPackages\ArenaShooter-20260830-173452-6ce5ed6`
- 검증 실행 파일: `C:\ArenaShooterPackages\ArenaShooter-20260830-173452-6ce5ed6\Windows\ArenaShooter\Binaries\Win64\ArenaShooter-Win64-Shipping.exe`
- 배포 ZIP: `C:\ArenaShooterPackages\ArenaShooter-v1.0.0-Windows.zip`
- ZIP 구성: 71개 엔트리, PDB 0개, Shipping EXE 포함
- ZIP 크기: 331,593,727 bytes
- ZIP SHA-256: `9E130D61386E3CE3535DB9AC0DF9F9F372777229E3763ECD060D18182F945291`
- Shipping 실행 파일 SHA-256: `5548A2A66376D70A475283B75BA803A3D7E470A048D81FBC9CCF18AEAEBE20D4`

이 패키지는 리팩터링과 Boss 표현 변경이 포함된 커밋을 ASCII detached worktree에서 직접 빌드한 결과다. 이후 문서 기록 커밋은 패키지 바이너리에 영향을 주지 않으므로, Release 자산의 소스 커밋은 `6ce5ed6`으로 고정한다.

## 7. 2026-08-31 Title 포함 최종 포트폴리오 빌드

- 소스 커밋: `500d2d9` (`add packaging affinity recovery`), Title 구현 커밋 `70cee67` 포함
- i7-14700K 복구 실행: 격리 filesystem DDC, Cook `-corelimit=8`, IoStore/UnrealPak `-corelimit=2`, 검증한 E-core affinity 사용
- Win64 Shipping/Editor 컴파일과 UnrealHeaderTool 성공
- Cook 565개 패키지, 오류 0 / 경고 0
- Stage/Pak/IoStore/Archive 성공, IoStore 셰이더 재압축 성공
- 실제 `ArenaShooter-Win64-Shipping.exe`에서 Round 1~5 스모크 종료 코드 0
- 성공 마커: `CWS_PACKAGE_VERIFICATION_SUCCESS: existing Shipping package passed the all-round smoke test.`
- 복구 원본 Stage: `C:\ArenaShooterPackageWork\ArenaShooter-20260830-235653-500d2d9\Saved\StagedBuilds`
- 검증 실행 파일: `C:\ArenaShooterPackageWork\ArenaShooter-20260830-235653-500d2d9\Saved\StagedBuilds\Windows\ArenaShooter\Binaries\Win64\ArenaShooter-Win64-Shipping.exe`
- 배포 ZIP: `C:\ArenaShooterPackages\ArenaShooter-v1.0.0-Windows.zip`
- ZIP 구성: 71개 엔트리, PDB 0개, 런처 1개, Shipping EXE 1개
- ZIP 크기: 334,856,723 bytes
- ZIP SHA-256: `14E78124DE6263E9A2D07368B70ECB19FBB1DACAA01D167BCB0D393FB2AA1CD9`
- Shipping 실행 파일 SHA-256: `90A1C560DB52D1E5EA35C159A915C75BC8DB6653AE6D67C07038CF7E8BDB90F9`
- 런처 SHA-256: `E2BA1F2F1728F81849CCD44D5B8CB7F6FA49735576B948ED38581CCFC747848A`
- 복구 요청 시 기존 canonical ZIP과 Archive가 경로에서 확인되지 않아 보존 Stage에서 새로 생성

Warm DDC와 무제한 IoStore 실행에서는 Oodle/`FLargeMemoryReader` 손상이 재현됐고, 한 차례 UHT 접근 위반도 발생했다. 시스템 캐시는 삭제하지 않고 별도 백업으로 보존했다. 격리 DDC와 단계별 코어 제한, E-core affinity를 적용한 실행에서 Cook과 IoStore가 완료됐다. 처음 패키지 스모크는 빌드용 affinity를 상속해 60초 제한을 넘었지만, 동일 Archive를 정상 affinity로 재검증하자 7.3초에 종료 코드 0으로 완료됐다. 이후 스크립트는 패키지 생성 직후 affinity를 복원하고 스모크를 실행하도록 수정했다.

이후 패키징 스크립트와 문서만 보완했으며 게임 바이너리는 바뀌지 않았으므로 Release 자산의 소스 커밋은 `500d2d9`로 기록한다.

2026-08-31 재생성본은 .NET `ZipArchive`로 작성했다. 71개 엔트리와 PDB 0개를 확인한 뒤 별도 폴더에 전체 압축 해제했고, 28개 파일의 길이와 SHA-256을 원본 Stage와 모두 대조했다. `tar.exe -tf` 교차 검사와 압축 해제본의 실제 Shipping Round 1~5 스모크도 통과했다.
