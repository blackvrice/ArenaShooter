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
