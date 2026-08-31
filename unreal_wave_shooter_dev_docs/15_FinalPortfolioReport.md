# 15. 최종 포트폴리오 보고서

이 문서는 `master`의 ArenaShooter를 넥슨 넥토리얼 신입 게임 프로그래머 포트폴리오 제출 수준으로 정리한 최종 결과다. 프로젝트는 **Human-directed, AI-assisted, Test-verified** 방식으로 진행했으며, 개발자가 요구사항·게임 규칙·합격 기준·수정 방향과 최종 실행 검증을 결정했다.

## 1. 변경 전 가장 큰 문제 5개

1. `ACWSGameMode`에 실제 게임 흐름과 Round/Balance/Screenshot 테스트 구현, 프로세스 종료 로직이 함께 있어 책임과 수명이 얽혀 있었다.
2. Boss가 안정성을 위해 Normal 메시 폴백을 사용하면서 영상에서 단순 확대 적처럼 보일 가능성이 컸다.
3. 탄약 총량 210발로는 적 97기를 처치하는 데 필요한 최소 404발을 충족할 수 없어 실제 5라운드 완주가 불가능했다.
4. README가 기능 목록 중심이라 채용 담당자가 5~10초 안에 장르, 엔진, C++, 핵심 기술과 검증 수준을 파악하기 어려웠다.
5. Editor 성공과 Shipping 성공 사이에 Niagara 직렬화, 한글 경로 SharedPCH, DDC/Oodle 손상처럼 빌드만으로 드러나지 않는 런타임·배포 위험이 있었다.

## 2. 변경한 내용

- GameMode의 테스트 상태와 단계별 구현을 Coordinator와 Runner 3종으로 분리했다.
- 기존 명령행 옵션과 PowerShell 진입점을 유지하고 실제 Weapon/Enemy/Wave 객체를 계속 사용했다.
- 실제 `TryFire()` 경로로 라운드별 `24 / 40 / 104 / 132 / 104`, 총 404발을 검증하도록 탄약 경제를 조정했다.
- Boss에 Crown, Aura Ring, Point Light와 보라→주황→적색 페이즈 표현을 추가했다.
- 저장된 맵을 수정하지 않고 중앙 링, 충돌 엄폐물, 방향 비콘을 런타임 레이어로 구성했다.
- Fire/Hit/Death/Reload/Enemy Attack/Boss Attack/Supply 피드백을 기존 시스템에 연결하고 회귀검증에 포함했다.
- Title Canvas 화면과 Enter 시작 흐름을 추가하고 Title 대기 중 게임 입력과 웨이브 시작을 막았다.
- README를 게임 개요, 핵심 플레이, 기술 Highlights, 실제 Problem Solving, AI 활용, 실행·검증 순서로 재구성했다.
- ASCII detached worktree, 격리 DDC, Cook/UnrealPak 코어 제한과 선택형 affinity를 지원하는 Shipping 자동화를 보강했다.
- Title 포함 Windows Shipping ZIP을 생성하고 전체 압축 해제, 파일별 해시 대조, 압축 해제본 Round 1~5 실행까지 검증했다.

## 3. CWSGameMode 리팩터링 전/후 책임 비교

| 구분 | 변경 전 | 변경 후 |
|---|---|---|
| 런타임 흐름 | Player/Wave 연결, Round Clear, Game Over, Restart | 그대로 유지하고 Title 대기/시작만 추가 |
| 테스트 선택 | GameMode가 명령행 옵션을 직접 해석 | `FCWSGameplayTestCoordinator::StartFromCommandLine()`에 위임 |
| Round 검증 | GameMode 내부 타이머와 상태 | `FCWSCombatSmokeRunner` |
| 탄약 밸런스 | GameMode 내부 실제 사격 상태 | `FCWSBalanceTestRunner` |
| 캡처 QA | GameMode 내부 HUD/전투/공격/비주얼 상태 | `FCWSScreenshotTestRunner` |
| 프로세스 종료 | GameMode 곳곳에서 직접 처리 | 각 Runner가 자신의 성공/실패 종료를 소유 |
| 수명 관리 | 레벨 재시작 뒤 오래된 타이머 위험 | GameMode weak lambda와 Coordinator 소유 범위로 제한 |
| 구현 크기 | 약 1,660줄 CPP / 204줄 Header | 현재 246줄 CPP / 93줄 Header(Title 흐름 포함) |

GameMode는 테스트를 시작할지 판단하도록 Coordinator를 호출하고, 런타임 이벤트를 좁게 전달한다. 테스트 단계, 기대값, 타임아웃, 성공 마커와 종료 코드는 Runner 내부에 남는다.

## 4. 새로 만든 클래스와 각 책임

| 클래스 | 위치 | 책임 |
|---|---|---|
| `FCWSGameplayTestCoordinator` | `Source/ArenaShooter/Private/Tests/CWSGameplayTestCoordinator.*` | 명령행 테스트 모드 선택, Runner 생성, GameMode 이벤트 전달, Title 자동 통과 여부 결정 |
| `FCWSCombatSmokeRunner` | `Source/ArenaShooter/Private/Tests/CWSCombatSmokeRunner.*` | Round 1/1~5, Player Death, Restart, 보급, 적 타입, Boss 패턴, 전투 피드백 검증 |
| `FCWSBalanceTestRunner` | `Source/ArenaShooter/Private/Tests/CWSBalanceTestRunner.*` | 실제 camera ray, `TryFire()`, 발사 간격, Reload, Supply를 이용한 404발 탄약 경제 검증 |
| `FCWSScreenshotTestRunner` | `Source/ArenaShooter/Private/Tests/CWSScreenshotTestRunner.*` | Title/HUD/Combat/Attack/Visual 캡처 상태 구성, PNG signature/IEND 무결성, 시각 상태 검증 |

테스트 클래스는 `Private/Tests`에 두어 Production 모듈의 공개 API로 노출하지 않았다.

## 5. 유지된 자동 테스트 목록

- Round 1 Smoke Test
- Round 1~5 All Rounds Smoke Test
- Player Death / Game Over / Restart
- Normal / Fast / Tank / Boss 타입과 수치
- Boss 3 Phase / Ground Slam / Shockwave / Knockback
- Fire / Hit / Death / Enemy Attack / Boss Explosion 피드백
- 시간 기반 Reload / Ammo Supply / Health Supply
- 실제 Hitscan 기반 Balance Combat Test
- Title → Round 1 시작 흐름
- Title / HUD / Combat / Attack / Visual Polish Screenshot QA
- PNG signature와 `IEND` 청크 무결성
- Map Check / Spawn Point / Round 구성 / 입력 매핑 정적 검사
- Win64 Shipping Build/Cook/Stage/Pak/IoStore/Archive
- 패키지 실행 파일 Round 1~5 Smoke Test
- 배포 ZIP 전체 압축 해제와 파일별 SHA-256 대조

## 6. 실제 실행한 테스트와 결과

| 검증 | 실제 결과 |
|---|---|
| UnrealHeaderTool / Editor Development | 성공 |
| Map Check | 오류 0 / 경고 0 |
| Spawn/Wave | SpawnPoint 9개, `8 / 16 / 24 / 34 / 15`, 총 97기 |
| Round 1 | `CWS_ROUND_ONE_SMOKE_SUCCESS` |
| Round 1~5 | `CWS_ALL_ROUNDS_SMOKE_SUCCESS` |
| Balance | 97기, 404명중, 0 miss, 70% 기준 가용 600/필요 578, 잔탄 196 |
| Title 캡처 | 245,123 bytes, Title idle과 Title→Round 1 확인 |
| HUD 캡처 | 841,432 bytes |
| Combat 캡처 | 940,429 bytes |
| Attack 캡처 | 941,238 bytes |
| Visual 캡처 | 1,100,380 bytes |
| Shipping Cook | 565개, 오류 0 / 경고 0 |
| Shipping 파이프라인 | Build/Cook/Stage/Pak/IoStore/Archive 성공 |
| Packaged EXE | 실제 Shipping Round 1~5 종료 코드 0 |
| 재생성 ZIP | 71 entries, PDB 0, 실제 파일 28개 전체 해시 일치 |
| ZIP 압축 해제본 | 실제 Shipping Round 1~5 종료 코드 0 |

최종 ZIP은 `C:\ArenaShooterPackages\ArenaShooter-v1.0.0-Windows.zip`, 334,856,723 bytes이며 SHA-256은 `14E78124DE6263E9A2D07368B70ECB19FBB1DACAA01D167BCB0D393FB2AA1CD9`다.

## 7. 수정 과정에서 발견한 버그

1. 분리한 Runner의 raw timer가 레벨 재시작 뒤 파괴된 객체를 호출했다.
2. 불완전한 `TUniquePtr` 타입을 UCLASS 헤더가 소유해 생성 코드에서 파괴자를 인스턴스화할 때 빌드가 실패했다.
3. Balance 표적이 Pawn view 기준이라 숄더 카메라의 실제 라인트레이스와 어긋나 420발이 모두 빗나갔다.
4. 비동기 캡처 완료 전에 파일 크기만 검사해 header가 0인 PNG를 성공으로 판정했다.
5. Title 시작 연결 뒤 `BindGameplayActors()`가 반복 호출될 때 웨이브 시작을 두 번 발행해 라운드별 명중 수가 정확히 2배가 됐다.
6. Editor에서 동작한 Niagara 피격/사망 효과가 Shipping 기본 맵 로드 중 직렬화 접근 위반을 일으켰다.
7. 한글 프로젝트 경로에서 Shipping SharedPCH 소스를 찾지 못했다.
8. Warm DDC와 고병렬 IoStore에서 Oodle/`FLargeMemoryReader` 손상 및 UHT 접근 위반이 간헐적으로 발생했다.
9. 최초 재생성 ZIP/Archive가 외부 정리 뒤 경로에서 사라져 보존 Stage에서 배포 ZIP을 다시 만들어야 했다.

## 8. 해결한 문제와 해결 방법

| 문제 | 해결 | 검증 |
|---|---|---|
| Runner 수명 | `CreateWeakLambda`, World 파괴 뒤 접근 제거 | Restart 포함 Round Smoke |
| UCLASS와 불완전 타입 | GameMode에는 private raw pointer와 명시적 파괴자, Runner 내부에는 `TUniquePtr` 유지 | UHT/Editor 빌드 |
| 실제 Hitscan miss | `PlayerController` camera view ray 위에 표적 배치 후 다음 프레임 발사 | 404 hit / 0 miss |
| PNG 조기 성공 | PNG signature와 끝의 `IEND`가 기록될 때까지 최대 10초 대기 | 캡처 5종 유효 파일 |
| 웨이브 이중 시작 | `bWaveStartIssued`와 `TryStartWaveSystem()`으로 게임당 한 번만 발행 | Balance와 All Rounds 재검증 |
| Shipping Niagara 충돌 | Native mesh/light 기반 `ACWSCombatBurstEffect`로 교체 | Packaged EXE Round 1~5 |
| Additive rifle fire | 전신 단독 재생을 제거하고 부착 라이플 C++ recoil로 대체 | 실제 사격과 Combat 캡처 |
| 한글 경로 UBT | 커밋된 HEAD를 ASCII detached worktree에서 패키징 | Shipping Build 성공 |
| DDC/Oodle 불안정 | 시스템 캐시 보존, 격리 filesystem DDC, Cook 8코어, UnrealPak 2코어, 선택형 affinity | Cook 565와 IoStore 성공 |
| ZIP 손상 우려 | .NET `ZipArchive`, 전체 압축 해제, 28개 파일별 해시 비교, 압축 해제본 실행 | Shipping Smoke 성공 |

## 9. 아직 남은 기술 부채

- 일부 적의 additive 피격 애니메이션을 standalone 방식으로 재생할 때 cooked build 경고가 남는다. 전용 AnimBP additive slot 또는 비-additive hit reaction이 필요하다.
- 일부 Skeletal Mesh의 derived data key가 로드 뒤 달라져 첫 DDC 구축 시간이 길다. UE 5.6 안전 재저장과 에셋 단위 회귀검증이 필요하다.
- Canvas HUD는 의존성이 작고 Shipping에 안정적이지만, 다양한 해상도·현지화·접근성 확장에는 UMG 기반 레이아웃보다 불리하다.
- Boss는 Native primitive로 명확히 구분되지만 전용 제작 메시와 애니메이션을 사용한 상용 수준의 아트 완성도에는 미치지 못한다.
- Gameplay GIF와 실제 사람 조작 60~90초 영상은 아직 촬영하지 않았다.
- GitHub CLI가 인증되지 않아 `v1.0.0` Release 페이지와 ZIP 첨부는 생성하지 못했다.
- ZIP은 동일 PC의 별도 폴더 압축 해제본으로 실행 검증했으며, 깨끗한 PC 또는 별도 Windows 계정 QA는 남아 있다.
- i7-14700K 호스트의 간헐적 UHT/Oodle 불안정은 프로젝트 코드가 아닌 빌드 환경 위험으로, BIOS/microcode와 하드웨어 안정성 점검이 별도로 필요하다.

## 10. 채용 담당자가 볼 만한 핵심 기술 포인트 5개

1. **실제 경로 검증:** 수식 비교가 아니라 `TryFire()`→Visibility line trace→Damage→Death→Wave 진행을 404발로 검증했다.
2. **책임 분리:** 1,600줄대 GameMode 테스트 구현을 Private Coordinator/Runner 구조로 이동하면서 기존 명령과 검증 범위를 유지했다.
3. **상태 기반 웨이브:** 9개 스폰 지점, 타입별 pending queue, 생존 적 이벤트와 Round phase를 이벤트 중심으로 연결했다.
4. **실패에서 출발한 Shipping 안정화:** Niagara 직렬화, 한글 경로, DDC/Oodle, 압축본 무결성 문제를 실제 패키지 실행까지 추적했다.
5. **표현과 테스트의 결합:** Boss 페이즈 색·Crown·Aura와 Title 흐름을 자동 상태 검사와 사람이 보는 PNG QA 양쪽으로 검증했다.

## 11. 면접에서 질문 받을 가능성이 높은 내용 10개

1. GameMode에서 테스트를 왜 UObject/Actor가 아닌 plain C++ Runner로 분리했는가?
2. 테스트가 Production gameplay 상태를 복제하지 않는다는 것을 어떻게 보장했는가?
3. 404발과 600발이라는 탄약 수치는 어떻게 계산하고 실제로 검증했는가?
4. Title 추가가 왜 웨이브 이중 시작을 만들었고 어떤 invariant로 막았는가?
5. 레벨 재시작 시 timer callback 수명 문제를 어떻게 해결했는가?
6. Boss의 3 Phase와 두 패턴, 시각 표현을 어떤 이벤트로 연결했는가?
7. Additive 애니메이션을 전신 재생하지 않은 이유와 현재 recoil 대안은 무엇인가?
8. Editor에서는 정상인데 Shipping에서 Niagara가 충돌한 원인을 어떻게 좁혔는가?
9. 한글 경로와 DDC/Oodle 불안정을 빌드 자동화에서 어떻게 격리했는가?
10. AI를 어디까지 사용했고 개발자가 직접 결정·검증한 부분은 무엇인가?

## 12. 각 질문에서 반드시 이해해야 할 코드 위치

| 질문 | 코드 위치와 핵심 심볼 |
|---|---|
| 1. Runner 설계 | `Private/Tests/CWSGameplayTestCoordinator.cpp::StartFromCommandLine`, 각 Runner header의 소유 상태 |
| 2. 실제 gameplay 사용 | `CWSCombatSmokeRunner.cpp`, `CWSBalanceTestRunner.cpp::StartFromCommandLine`, `CWSHitscanWeaponComponent.cpp::TryFire` |
| 3. 탄약 경제 | `Public/Components/CWSHitscanWeaponComponent.h`의 60/360/480 설정, `CWSBalanceTestRunner.cpp`의 라운드별 hit 집계 |
| 4. 웨이브 중복 방지 | `Private/Game/CWSGameMode.cpp::StartGame`, `TryStartWaveSystem`, `Public/Game/CWSGameMode.h::bWaveStartIssued` |
| 5. Timer 수명 | `Private/Game/CWSGameMode.cpp::BeginPlay`, `BindGameplayActors`, Coordinator/Runner의 callback 위임 |
| 6. Boss | `Private/Enemy/CWSBossEnemy.cpp::UpdateBossPhase`, `ApplyBossPhasePresentation`, `ExecuteGroundSlam`, `ExecuteShockwave` |
| 7. Rifle recoil | `Private/Player/CWSPlayerCharacter.cpp`의 fire 입력과 recoil, `Private/Components/CWSHitscanWeaponComponent.cpp::TryFire` |
| 8. Shipping VFX | `Private/Feedback/CWSCombatBurstEffect.cpp`, `Private/Enemy/CWSEnemyBase.cpp`의 hit/death feedback 호출 |
| 9. 패키징 복구 | `Tools/Build/run_package_windows.ps1`의 ASCII worktree, `ColdDdc`, `RecoveryPakCoreLimit`, `RecoveryAffinityMask` |
| 10. AI와 검증 책임 | `README.md::AI-assisted Development`, `12_CompletionProgressLog.md`, 이 보고서의 실제 테스트 결과 |

## 13. 100점 기준 자체 평가

| 항목 | 점수 | 근거 |
|---|---:|---|
| Gameplay 완결성 | 18/20 | 5라운드, 4종 적, Boss, 보급, Game Over/Restart/Final Clear 완주 가능. 콘텐츠 폭은 의도적으로 제한 |
| 코드 구조 | 18/20 | GameMode 책임 분리와 Private Runner 경계가 명확함. 일부 Canvas HUD와 테스트 orchestration은 더 일반화할 수 있으나 현재 규모에는 적절 |
| 자동 검증 | 20/20 | 실제 gameplay 경로, 캡처 무결성, Shipping EXE, 압축 해제본까지 검증 |
| 시각·포트폴리오 표현 | 15/20 | Title과 Boss 차별화, 캡처는 완료. 실제 Gameplay 영상/GIF와 전용 Boss 아트가 없음 |
| Release 완성도 | 16/20 | 검증 ZIP·해시·Release notes 준비 완료. GitHub Release 게시와 깨끗한 PC QA가 남음 |
| **총점** | **87/100** | 신입 C++ 게임 프로그래머 포트폴리오로 기술적 증거는 강하지만, 최종 영상과 외부 배포 검증을 마쳐야 제출 완성도가 올라감 |

가장 큰 감점은 코드 기능이 아니라 **실제 플레이 영상 부재, GitHub Release 미게시, 전용 아트 부족**이다. 60~90초 영상과 Release 업로드, 깨끗한 Windows 환경 실행을 완료하면 코드 변경 없이도 제출 완성도를 약 92점 수준까지 높일 수 있다.

