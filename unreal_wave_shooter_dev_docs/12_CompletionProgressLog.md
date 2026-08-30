# 남은 개발 순차 진행 기록

남은 작업은 각 항목을 `분석 → 구현 → 빌드 → 런타임 검증 → 기록 → 커밋/푸시` 순서로 완료한다. 사용자 소유의 기존 맵/에디터 변경은 건드리거나 커밋하지 않는다.

## 2026-08-25 - 1단계: 적 공격 애니메이션과 전투 SFX

상태: 완료

### 분석

- `MM_Attack_01` 공격 애니메이션은 프로젝트에 있었지만 `TryAttack()` 경로에 연결되지 않았다.
- 발사/피격/폭발용 SoundWave 또는 원본 오디오 파일이 없었다.
- Round 1과 전체 라운드 스모크는 기존 피격/사망 VFX만 검사하고 공격 연출과 음향은 검사하지 않았다.

### 구현

- 일반 적의 유효 근접 공격에 `MM_Attack_01` 동적 몽타주와 근접 공격음을 연결했다.
- 보스 Ground Slam과 Shockwave에 공격 몽타주와 저역 폭발음을 연결했다.
- 총기 발사 위치와 실제 라인트레이스 충돌 위치에 각각 발사음과 충돌음을 연결했다.
- `UCWSCombatSoundWave`가 44.1 kHz mono PCM을 런타임 합성하도록 구현해 외부 음원 없이 Shipping에서도 동일한 경로를 사용한다.
- 발사음, 충돌음, 일반 적 공격 피해/몽타주/음향, 보스 폭발음을 스모크 성공 조건에 추가했다.
- `run_attack_feedback_screenshot.ps1`를 추가해 실제 오디오 장치와 공격 자세를 함께 검증한다.

### 검증

- UnrealHeaderTool 및 `ArenaShooterEditor Win64 Development` 비유니티 빌드 성공.
- Round 1: `CWS_COMBAT_FEEDBACK_SMOKE_STATE: FireSound=true ImpactSound=true ...` 확인.
- Round 1: `CWS_ENEMY_ATTACK_FEEDBACK_VERIFIED`와 `CWS_ROUND_ONE_SMOKE_SUCCESS` 확인.
- Round 1~5: `CWS_BOSS_SMOKE_VERIFIED`의 폭발음 항목과 `CWS_ALL_ROUNDS_SMOKE_SUCCESS` 확인.
- 렌더링 실행: XAudio2 오디오 장치 초기화 후 `CWS_ATTACK_FEEDBACK_VERIFIED` 확인.
- 시각 QA: `CWSEnemyAttackFeedback.png` 1280×720, 939,566 bytes 생성 및 공격 자세 직접 확인.
- Warm DDC 손상 시 Cold DDC 1회 재시도 경로를 캡처 스크립트에 포함했다.

### 다음 순서

2단계: 맵 디테일링과 Normal/Fast/Tank 시각 구분.

## 2026-08-25 - 2단계: 맵 디테일링과 적 타입 시각 구분

상태: 완료

### 분석

- 저장된 맵은 넓은 전투 공간을 갖췄지만 중앙 교전 지점, 일반 엄폐물, 방향 표식이 부족했다.
- Normal/Fast/Tank는 크기와 능력치가 달라도 전투 중 즉시 읽을 수 있는 타입 표식이 없었다.
- World Partition 외부 액터에 사용자 변경이 있어 맵 에셋을 재저장하지 않는 구현 경로가 필요했다.

### 구현

- `ACWSArenaVisualDirector`가 중앙 링 24조각, 실제 충돌·내비게이션 엄폐물 8개, 동서남북 게이트 비콘 8개를 런타임에 생성하도록 했다.
- Normal은 초록, Fast는 주황, Tank는 파랑, Boss는 보라 마커와 발밑 밴드를 사용하도록 했다.
- Round 1과 전체 라운드 스모크 성공 조건에 아레나 구성 수량과 타입별 표시 색상을 추가했다.
- `run_visual_polish_screenshot.ps1`를 추가해 세 일반 타입과 아레나 레이어를 한 화면에서 검사한다.
- 저장된 `Lvl_Combat`와 사용자 소유 외부 액터는 수정하거나 커밋 대상에 포함하지 않았다.

### 검증

- `ArenaShooterEditor Win64 Development` 빌드 성공.
- Round 1: 아레나 구성, Normal 초록 표시, 전투·재시작 흐름과 `CWS_ROUND_ONE_SMOKE_SUCCESS` 확인.
- Round 1~5: Normal/Fast/Tank 표시와 기존 전체 전투 흐름, `CWS_ALL_ROUNDS_SMOKE_SUCCESS` 확인.
- 렌더링 실행: `CWS_VISUAL_POLISH_VERIFIED`와 `CWS_VISUAL_POLISH_SCREENSHOT_SUCCESS` 확인.
- 시각 QA: `CWSArenaVisualPolish.png` 1280×720, 1,047,028 bytes에서 초록 Normal, 주황 Fast, 파랑 Tank, 엄폐물, 중앙 링 일부와 방향 비콘을 직접 확인.
- 첫 캡처에서 엄폐물 뒤에 가려진 Normal을 발견해 타입 간격과 카메라 전방 거리를 조정한 뒤 재검증했다.

### 다음 순서

3단계: Round 1~5 탄약 경제, 적 체력과 교전 압력 밸런싱.

## 2026-08-25 - 3단계: Round 1~5 실제 사격 밸런싱

상태: 완료

### 분석

- 적 97명의 현재 체력과 무기 데미지 25를 적용하면 라운드별 완벽 명중 필요 탄수는 `24 / 40 / 104 / 132 / 104`, 총 404발이다.
- 기존 탄약은 탄창 60 + 예비 90 + 탄약 보급 30×2 = 최대 210발이라 실제 5라운드 완주가 불가능했다.
- 적 수, 타입별 체력, 공격력과 보스 체력은 이미 역할 구분이 명확해 탄약 경제를 우선 조정하는 것이 적절했다.

### 구현

- 시작 예비 탄약을 360발, 최대 예비 탄약을 480발로 조정했다.
- Round 1·3 탄약 보급을 각각 90발로 조정해 총 탄약 예산을 600발로 만들었다.
- 70% 명중률에서 필요한 578발보다 22발의 여유가 생기도록 목표를 고정했다.
- `run_balance_combat_test.ps1`와 전용 런타임 경로를 추가했다. 이 경로는 적을 즉사시키지 않고 실제 `TryFire()`, 라인트레이스 데미지, 발사 간격, 재장전, 보급 수집으로 Round 1~5를 진행한다.

### 검증

- `ArenaShooterEditor Win64 Development` 빌드 성공.
- 첫 실제 사격 실행은 새 표적 조준과 발사가 같은 프레임에 일어나 Round 1에서 1발이 빗나갔고, `405발/빗나감 1`로 실패 처리됐다.
- 표적 이동과 조준 뒤 1프레임 예열을 추가하고 재실행해 `24 / 40 / 104 / 132 / 104`를 정확히 확인했다.
- 최종 실제 사격 실행: 97명 처치, 404발 명중, 빗나감 0, 탄약 보급 2회, 체력 보급 2회, 종료 잔탄 196발.
- `CWS_BALANCE_COMBAT_SUCCESS` 확인.
- 기존 Round 1 전투·사망·재시작 회귀 테스트 통과.
- 전체 Round 1~5 회귀 테스트는 Warm DDC `BufferReader` 손상 뒤 격리 Cold DDC에서 `CWS_ALL_ROUNDS_SMOKE_SUCCESS` 확인.

### 다음 순서

4단계: 자동화 러너의 DDC 재시도 누락을 포함한 전체 버그 수정과 회귀검증.

## 2026-08-25 - 4단계: 자동화 안정화와 전체 회귀검증

상태: 완료

### 분석

- Unreal 5.6의 Warm DDC에서 `OodleLZ_Decompress`와 `BufferReader.h` 오류가 간헐적으로 발생해 게임 로직 진입 전에 프로세스가 종료됐다.
- 일부 러너는 성공/실패 마커가 없는데도 알려진 로그 문구가 없으면 Cold DDC 재시도를 수행하지 않았다.
- 세 스크린샷 러너의 재시도 함수가 전달받은 DDC 그래프 대신 다시 Warm을 사용해 격리 재시도가 실제로 적용되지 않았다.
- Python 검사 커맨드렛은 병렬 워커에서 같은 오류가 발생했으며, 렌더 캡처는 Cold DDC를 즉시 사용하면 전체 셰이더 재생성으로 회귀 시간이 과도하게 늘어났다.

### 구현

- Round 1/전체 라운드와 실제 사격 러너는 결과 마커가 없으면 Cold DDC로 한 번 재시도하고, 게임 실행에는 `-ReduceThreadUsage`를 적용했다.
- 네 Python 검사 러너는 성공 마커를 종료 코드와 독립적으로 판정하고 `-NoLoadStartupPackages -nothreading`을 적용했다. 알려진 DDC 손상 시 Warm 캐시를 유지하며 최대 3회 재시도한다.
- HUD, 전투 피드백, 적 공격 피드백, 시각 폴리시 캡처는 `-NoLoadStartupPackages`를 사용하고 Warm DDC를 최대 3회 누적 재시도한 뒤에만 Cold DDC로 전환한다.
- HUD/전투/시각 캡처의 Cold 재시도가 전달된 DDC 그래프를 실제 실행에 사용하도록 수정했다.
- 모든 변경 PowerShell 러너를 구문 분석해 문법 오류가 없음을 확인했다.

### 검증

- 정적 검사 4종 통과: 맵 체크 오류 0/경고 0, 스폰 지점 9개와 총 97기, Boss 클래스, `IMC_Combat` 유효 매핑 14개.
- Round 1: `CWS_ROUND_ONE_SMOKE_SUCCESS` 확인.
- Round 1~5: `CWS_ALL_ROUNDS_SMOKE_SUCCESS` 확인. 아레나, 타입 색상, 전투 피드백, 재장전, 보급, 라운드 단계, Fast/Tank, Boss 패턴과 최종 클리어 포함.
- 실제 사격 밸런스: 97기, 실제 명중 404발, 빗나감 0, 70% 기준 가용 600/필요 578발, 완벽 진행 잔탄 196발, 탄약/체력 보급 각 2회.
- 오프스크린 캡처 4종 성공: HUD 1,008,520 bytes, 전투 피드백 919,782 bytes, 적 공격 피드백 974,061 bytes, 시각 폴리시 1,040,634 bytes.
- 첫 HUD 캡처의 셰이더 노이즈를 시각 검수에서 발견해 캐시 완료 후 재캡처했으며, 최종 이미지에서 상태 HUD·Round 배너·조작 안내의 정상 표시를 직접 확인했다.
- 사용자 소유 `Config`, World Partition 외부 액터, PPTX와 IDE 파일은 수정 범위와 커밋에서 제외했다.

### 다음 순서

5단계: 최신 코드 기준 Win64 Shipping 출시 후보 패키지, 실행 검증, README/기술 문서와 촬영 체크리스트 정리.

## 2026-08-27 - 5단계: Shipping 출시 후보와 포트폴리오 정리

상태: 완료

### 분석과 수정

- 첫 최신 Shipping 패키지는 Build/Cook/Stage/Pak/Archive에는 성공했지만 기본 맵 로드에서 종료 코드 3으로 실패했다.
- Shipping PDB와 CrashContext를 심볼화해 `UNiagaraStatelessEmitter::Serialize` 접근 위반을 확인했다.
- `NS_Damage`를 UE 5.6으로 재저장해도 같은 충돌이 재현되어 에셋 포맷 갱신만으로는 해결되지 않았다.
- 피격/사망 효과를 `ACWSCombatBurstEffect`의 확장 메시와 감쇠 포인트 라이트로 교체하고 Niagara 기본 객체 참조와 모듈 의존성을 제거했다.

### 검증과 배포

- 네이티브 버스트 적용 후 Editor Development 빌드 성공.
- Round 1~5 전체 회귀와 피격/사망 오프스크린 캡처 통과, 밝은 청록 버스트를 직접 시각 확인.
- `32218bc` 기준 Shipping/Editor 컴파일, Cook 515개, Stage/Pak/IoStore/Archive 성공.
- 실제 Shipping 실행 파일에서 Round 1~5 전체 스모크 종료 코드 0과 `CWS_PACKAGE_VERIFICATION_SUCCESS` 확인.
- PDB 포함 Archive를 보존하고 PDB 제외 319,659,273 bytes 배포 ZIP과 SHA-256을 생성했다.
- GitHub README, 기술 문서 인덱스, 최신 패키징 결과, 포트폴리오 소개 문안과 촬영 체크리스트를 정리했다.
- 사용자 소유 Config, World Partition 외부 액터, PPTX와 IDE 파일은 수정 범위와 커밋에서 제외했다.

### 완료 상태

코드·자동 검증·Shipping 출시 후보·배포 ZIP·기술 문서·포트폴리오 페이지까지 완료했다. 실제 사람 조작이 필요한 영상 녹화와 최종 체감 QA는 `13_PortfolioAndRelease.md`의 체크리스트로 인계한다.

## 2026-08-30 - 6단계: NexTorial 포트폴리오 구조와 표현 최종 정리

상태: 완료

### 분석

- `ACWSGameMode`는 약 1,660줄의 구현과 204줄 헤더에 Runtime 흐름, Round 스모크, 전체 라운드, 밸런스, 캡처 4종, 프로세스 종료 상태가 함께 있었다.
- 기존 Boss는 체력·3단계 패턴은 명확했지만 Normal 메시 1.65배 폴백과 보라 마커만 사용해 영상에서 단순 확대 적처럼 보일 여지가 있었다.
- README는 구현 정보는 충분했지만 Gameplay 미디어 위치, 압축된 기술 포인트, 실제 Problem Solving, AI 활용과 사람의 검증 책임이 첫 화면에 드러나지 않았다.
- 캡처 테스트는 파일 크기만 확인해, 비동기 PNG 기록이 완료되기 전에 종료하면 0으로 채워진 손상 파일도 성공 처리할 수 있었다.

### 구현

- GameMode 테스트 코드를 `FCWSGameplayTestCoordinator`, `FCWSCombatSmokeRunner`, `FCWSBalanceTestRunner`, `FCWSScreenshotTestRunner`로 옮겼다.
- GameMode는 Player/Wave 연결, 보급, Game Over/Clear, Restart와 Runner 이벤트 전달만 남겨 구현 203줄, 헤더 79줄로 축소했다.
- Runner의 타이머는 `CreateWeakLambda`를 사용해 레벨 재시작·World 파괴 뒤 콜백이 오래된 GameMode에 접근하지 않게 했다.
- 밸런스 Runner의 표적은 실제 Player camera view ray 위 800cm에 놓아 숄더 카메라에서도 실제 `TryFire()` 라인트레이스가 결정적으로 맞게 했다.
- 캡처 Runner는 PNG signature와 마지막 `IEND` 청크가 기록될 때까지 기다리고, 10초 안에 완성되지 않으면 실패하도록 강화했다.
- Boss에 전용 Crown, Aura Ring, 고강도 Point Light를 추가하고 Phase 1 보라, Phase 2 주황, Final Phase 적색과 Aura 크기 변화를 연결했다. 외부 에셋은 추가하지 않았다.
- Visual Polish 캡처에 4종 적과 Final Phase Boss를 함께 배치해 시각적 차이를 자동 상태 검사와 직접 화면 QA 양쪽에서 확인하게 했다.
- README 상단을 채용 담당자용 개요로 재구성하고 실제 탄약 경제, Shipping Niagara 충돌, Rifle additive 문제 해결과 Human-directed / AI-assisted / Test-verified 과정을 기록했다.

### 수정 중 발견하고 해결한 회귀

- 첫 분리 빌드에서 UCLASS 헤더의 불완전한 `TUniquePtr` 타입이 생성 코드에서 파괴자를 인스턴스화해 실패했다. Coordinator 소유를 GameMode의 private raw pointer와 명시적 파괴자로 제한하고, Runner 내부는 `TUniquePtr`를 유지했다.
- 초기 `CreateRaw` 타이머가 레벨 재시작 뒤 파괴된 Runner를 호출했고, 이어 Runner 파괴자가 이미 파괴 중인 World timer를 정리하다 충돌했다. GameMode weak lambda로 전환하고 파괴자에서 World 접근을 제거했다.
- 분리 직후 밸런스 테스트가 숄더 카메라와 Pawn view 차이 때문에 420발을 모두 빗나갔다. 실제 PlayerController camera ray에 표적을 배치해 테스트 조건을 완화하지 않고 404발 실사격 경로를 복구했다.
- 첫 HUD 캡처는 1,376,155 bytes였지만 PNG header가 모두 0이었다. 완전한 파일 signature를 기다리는 검증으로 수정한 뒤 유효 PNG를 재생성했다.

### Editor 검증

- `ArenaShooterEditor Win64 Development` 최종 빌드 성공.
- Map Check 오류 0 / 경고 0, PlayerStart/NavMesh/GameMode 확인.
- 9개 SpawnPoint, `8 / 16 / 24 / 34 / 15`, 총 97기와 기본 타입 구성 확인.
- Round 1: 전투 피드백, Reload, Ammo/Health Supply, Player Death, Wave Stop, Restart와 `CWS_ROUND_ONE_SMOKE_SUCCESS` 확인.
- Round 1~5: Normal/Fast/Tank, Boss 체력 1200, Final Phase, Ground Slam/Shockwave/넉백/폭발음, Final Clear와 `CWS_ALL_ROUNDS_SMOKE_SUCCESS` 확인.
- 실제 사격: `24 / 40 / 104 / 132 / 104`, 총 404명중/0 miss, 가용 600/70% 필요 578, 잔탄 196, 보급 각 2회와 `CWS_BALANCE_COMBAT_SUCCESS` 확인.
- 유효 PNG와 직접 시각 QA: HUD 1,389,127 bytes, Combat 944,316 bytes, Attack 916,185 bytes, Visual Polish 1,284,710 bytes.
- Visual Polish에서 Normal 초록, Fast 주황, Tank 파랑, Final Boss 적색, Crown, 넓은 Aura Ring, Boss HP bar를 직접 확인했다.

### Shipping 검증과 배포 자산

- `6ce5ed6`을 ASCII detached worktree에 체크아웃해 Win64 Shipping Build/Cook/Stage/Pak/IoStore/Archive를 실행했다.
- Cook 565개 패키지와 AutomationTool 전체 단계가 성공했다.
- Archive의 실제 `ArenaShooter-Win64-Shipping.exe`에서 Round 1~5 스모크가 종료 코드 0으로 끝났다.
- `CWS_PACKAGE_VERIFICATION_SUCCESS`를 확인했다.
- `C:\ArenaShooterPackages\ArenaShooter-v1.0.0-Windows.zip`을 생성하고 71개 엔트리, PDB 0개, Shipping EXE 포함을 확인했다.
- ZIP 크기 331,593,727 bytes, SHA-256 `9E130D61386E3CE3535DB9AC0DF9F9F372777229E3763ECD060D18182F945291`을 기록했다.

### 남은 문제

- Normal/Fast/Tank의 additive 피격 애니메이션을 `AnimSingleNodeInstance`에 단독 재생할 때 cooked build 경고가 남는다. 피격 상태 카운터와 네이티브 버스트는 동작하지만, 전용 AnimBP additive slot 또는 비-additive 피격 시퀀스로 정리할 필요가 있다.
- 일부 Skeletal Mesh의 derived data key가 로드 후 달라져 Editor 첫 DDC 구축이 매우 오래 걸린다. 에셋을 UE 5.6에서 안전하게 재저장하고 별도 회귀검증해야 한다.
- Gameplay GIF와 Video는 실제 사람 조작 촬영이 필요하므로 README에는 깨진 링크 대신 TODO를 유지한다.
- 현재 GitHub CLI가 인증되지 않아 Release 페이지와 Tag 생성은 수행하지 않았다. 대신 검증 ZIP과 SHA-256, Release notes, 업로드 체크리스트를 준비했다.
