# ArenaShooter 포트폴리오와 출시 후보

## 한 줄 소개

Unreal Engine 5.6 C++로 구현한 3인칭 아레나 웨이브 슈터로, 8방향 스폰과 4종 적, 실제 탄약 경제, 3단계 보스전, 자동 회귀검증과 Shipping 배포 파이프라인을 하나의 플레이 가능한 수직 슬라이스로 완성했다.

## 담당 구현

- `ACWSWaveManager`: Round 1~5, 총 97기, 9개 스폰 지점과 타입별 구성
- `UCWSHitscanWeaponComponent`: 실제 라인트레이스, 발사 간격, 탄창/예비 탄약, 시간 기반 재장전
- `ACWSEnemyBase`와 파생 클래스: Normal/Fast/Tank/Boss 능력치, NavMesh 추적, 공격·피격·사망 피드백
- `ACWSBossEnemy`: 체력 비율 기반 3단계 페이즈, Ground Slam과 Shockwave, Crown/Aura/Light 페이즈 표현
- `ACWSArenaVisualDirector`: 저장 맵을 훼손하지 않는 중앙 링, 엄폐물, 방향 비콘 런타임 레이어
- `ACWSCombatBurstEffect`와 `UCWSCombatSoundWave`: Shipping 공통 네이티브 VFX와 런타임 PCM 전투 SFX
- `ACWSGameMode`와 `ACWSHUD`: 영상용 Title 대기, Enter 시작, 자동 테스트 우회, 재시작 시 즉시 전투 복귀
- `FCWSGameplayTestCoordinator`와 Runner 3종: GameMode와 분리된 실제 사격 404발, Round 1~5, 보스 패턴, 사망/재시작, 오프스크린 시각 QA
- Shipping 자동화: ASCII worktree Build/Cook/Pak과 패키지 실행 파일 Round 1~5 검증

## 증명 가능한 결과

| 항목 | 결과 |
|---|---|
| 라운드 구성 | `8 / 16 / 24 / 34 / 15`, 총 97기 |
| 실제 사격 | 404발 명중, 빗나감 0 |
| 70% 명중률 탄약 | 가용 600발, 필요 578발 |
| 자동 검증 | Round 1, Round 1~5, 밸런스, Title 포함 캡처 5종 통과 |
| Shipping | Build/Cook/Stage/Pak/IoStore/Archive 및 패키지 전체 라운드 통과 |
| 출시 후보 | `ArenaShooter-v1.0.0-Windows.zip` (`500d2d9`, Title 포함) |

## 60~90초 플레이 영상 시나리오

1. 0~6초: Title 화면을 3~5초 유지해 프로젝트명·UE 5.6/C++·5 Round·4 Enemy Type을 읽게 한 뒤 Enter로 시작한다.
2. 6~14초: Title이 사라지고 Round 1 HUD가 나타나는 전환, 중앙 링과 방향 비콘을 짧은 카메라 이동으로 보여준다.
3. 14~30초: Normal에게 2~3발 발사해 총성, 충돌음, 버스트, 피격 애니메이션과 사망 피드백을 연속으로 담는다.
4. 30~43초: 탄창을 소비한 뒤 재장전 UI와 예비 탄약 감소, 탄약 보급 획득을 보여준다.
5. 43~58초: 주황 Fast와 파랑 Tank를 같은 화면에 두고 속도·크기·체력 차이를 보여준다.
6. 58~80초: Crown과 보라 Aura로 Boss 등장을 보여주고, 주황/적색 페이즈 변화와 Ground Slam·Shockwave를 담는다.
7. 80~90초: Round 5 클리어 화면과 Enter 즉시 재시작 흐름으로 마무리한다.

## 촬영 QA 체크리스트

- 1920×1080, 60fps, 게임 오디오와 마우스 커서 미포함
- Title을 최소 3초 유지하고 `PRESS ENTER TO START`가 읽힌 뒤 입력과 Round 1 전환을 한 컷으로 연결
- HUD가 화면 밖으로 잘리지 않고 `HEALTH / AMMO / ROUND / ENEMIES`가 읽히는지 확인
- Normal 초록, Fast 주황, Tank 파랑과 Boss의 Crown·넓은 Aura가 영상 압축 후에도 1초 안에 구분되는지 확인
- Boss Aura/Point Light가 Phase 1 보라 → Phase 2 주황 → Final Phase 적색으로 바뀌는 장면 포함
- 피격 버스트와 사망 버스트가 각각 최소 한 번 선명하게 보이는지 확인
- 재장전 중 입력과 보급 획득 전후 탄약 수치가 연결되는지 확인
- 보스 패턴 두 종류와 최종 클리어가 포함되는지 확인
- Title 이외의 불필요한 멈춘 구간, 디버그 메시지, 셰이더 컴파일 노이즈를 최종 편집에서 제거

## 실행과 배포

검증된 실행 파일은 `C:\ArenaShooterPackages\ArenaShooter-20260830-235653-500d2d9\Windows\ArenaShooter.exe`다. 배포 자산은 Title을 포함하고 PDB를 제외한 `C:\ArenaShooterPackages\ArenaShooter-v1.0.0-Windows.zip`이며, 압축 해제 후 `Windows\ArenaShooter.exe`를 실행한다. 상세 해시는 `11_BuildAndPackaging.md`에 기록되어 있다.
