# 09. Development Roadmap

## 1. 1차 목표: 플레이 가능한 최소 버전

### Week 1

- [x] 프로젝트 생성
- [x] Third Person 기반 네이티브 Player 구성
- [x] 중앙 아레나 맵 Blockout 제작
- [x] 8방향 + 중앙 SpawnPoint 배치
- [x] PlayerStart + NavMesh Bounds 배치

### Week 2

- [x] HealthComponent 구현
- [x] Hitscan Weapon 구현
- [x] EnemyBase + AIController 구현
- [x] 적 피격/사망 처리

### Week 3

- [x] WaveManager 구현
- [x] Round 1~5 스폰 로직 구현
- [x] 남은 적 수 체크
- [x] 라운드 클리어 처리

### Week 4

- [x] Boss Round 구현
- [x] 기본 Canvas HUD 구현
- [x] 보급/회복 아이템 구현
- [x] 게임 오버/클리어 상태 표시
- [x] 플레이어 사망 시 웨이브 정지와 Enter 레벨 재시작

## 2. 2차 목표: 포트폴리오 완성도

### Week 5

- 맵 디테일링
- 무료 에셋 적용
- Material 통일
- 엄폐물 배치 조정

### Week 6

- 적 애니메이션 적용
- 피격/사망 VFX
- 발사/피격/폭발 사운드
- [x] 라운드 준비/시작/클리어 경고 UI

### Week 7

- 밸런싱
- 버그 수정
- 패키징
- 플레이 영상 촬영

### Week 8

- GitHub README 작성
- 기술 문서 정리
- 포트폴리오 페이지 작성

## 3. 우선순위

| 우선순위 | 작업 |
|---|---|
| P0 | 플레이어 이동/사격 |
| P0 | 적 스폰/사망 |
| P0 | 라운드 진행 |
| P0 | 게임 오버/클리어 |
| P1 | UI |
| P1 | 보스 |
| P1 | 아이템 |
| P2 | VFX/SFX |
| P2 | 맵 디테일 |
| P3 | 미니맵/레이더 |
| P3 | 추가 무기 |

## 4. 완료 기준

최소 완료 기준:

- 플레이어가 이동하고 사격 가능
- 적이 라운드별 방향에서 스폰
- 적이 플레이어를 추적
- 적 처치 시 남은 적 수 감소
- 1~5라운드 진행
- 보스 처치 시 클리어
- UI 표시
- 패키징 가능

## 5. 현재 검증 상태와 다음 목표

- [x] 에디터 Development 빌드
- [x] `Lvl_Combat` Map Check 오류 0
- [x] 실제 히트스캔 사격으로 적 피격/사망 검증
- [x] Round 1 적 8마리 생성, NavMesh 추적, 사망, 라운드 클리어 검증
- [x] 플레이어 사망, 웨이브 정지, 현재 레벨 재시작 검증
- [x] Round 1~5 전체 스폰/클리어와 최종 게임 클리어 자동 검증
- [x] `IMC_Combat`의 누락된 `IA_Dash` 손상 참조 제거 및 14개 매핑 재검사
- [x] 전용 Boss 클래스, 3단계 페이즈, Ground Slam/Shockwave 패턴
- [x] Boss 체력/페이즈 HUD와 `OnBossSpawned` 이벤트
- [x] Boss 생성·페이즈·패턴 피해·Round 5 클리어 런타임 자동 검증
- [x] 60발 탄창, 예비 탄약, 1.2초 재장전과 HUD 상태 표시
- [x] Round 1~4 탄약/체력 보급 생성·수집 런타임 자동 검증
- [x] Fast/Tank 전용 적 클래스와 Round 2~5 타입별 구성
- [x] Fast/Tank 클래스 생성·능력치·Round 1~5 클리어 런타임 자동 검증
- [x] 라운드 준비/전투/클리어/완료 페이즈와 중앙 경고 HUD, 오프스크린 스크린샷 검증
- [ ] Round 1~5 수동 플레이 밸런싱
- [ ] 적 애니메이션, 피격/사망 VFX와 전투 SFX
- [ ] 맵 디테일링, 엄폐물 배치와 시각적 타입 구분
- [x] Win64 Shipping Build/Cook/Stage/Pak/Archive와 패키지 전체 라운드 실행 테스트
