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
