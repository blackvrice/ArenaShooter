# Central Wave Shooter - Unreal Engine C++ Development Docs

이 문서 묶음은 AI가 Unreal Engine C++ 소스 작성을 도와줄 때 참고하기 위한 개발 기준 문서입니다.

## 프로젝트 개요

- 장르: 3인칭 웨이브 슈터
- 엔진: Unreal Engine 5.x
- 개발 방식: C++ 기반 + Blueprint 보조
- 핵심 플레이:
  - 플레이어는 중앙에서 시작한다.
  - 적은 라운드별로 지정된 방향에서 중앙으로 진입한다.
  - 플레이어는 중앙 거점을 유지하면서 사방에서 접근하는 적을 처치한다.
  - 5라운드에서는 중앙에 보스가 출현한다.

## 문서 구성

| 파일 | 목적 |
|---|---|
| 01_GameDesignSpec.md | 게임 전체 기획서 |
| 02_MapAndWaveMetrics.md | 맵 수치, 스폰 방향, 라운드 수치 |
| 03_CPPArchitecture.md | C++ 클래스 구조 |
| 04_DataTablesAndStructs.md | DataTable/Struct 설계 |
| 05_BlueprintInterop.md | Blueprint와 C++ 역할 분리 |
| 06_AIImplementationPlan.md | 적 AI, 보스 AI 구현 방향 |
| 07_UIAndGameFlow.md | UI, 게임 상태 흐름 |
| 08_AssetFolderGuide.md | 에셋/소스 폴더 구조 |
| 09_DevelopmentRoadmap.md | 개발 순서 |
| 10_AIPromptGuide.md | AI에게 코드 작성을 요청할 때 사용할 프롬프트 |
| 11_BuildAndPackaging.md | Windows Shipping 패키징과 배포본 검증 |
| 12_CompletionProgressLog.md | 남은 개발 항목의 순차 구현·검증 기록 |

## AI에게 우선 전달할 핵심 문서

AI에게 처음 코드 작성을 시킬 때는 아래 4개 문서를 먼저 전달하는 것을 추천합니다.

1. 01_GameDesignSpec.md
2. 02_MapAndWaveMetrics.md
3. 03_CPPArchitecture.md
4. 04_DataTablesAndStructs.md

## 추천 구현 순서

1. C++ 프로젝트 생성
2. Player Character 기본 이동/조준
3. HealthComponent 구현
4. EnemyBase 구현
5. SpawnPoint 구현
6. WaveManager 구현
7. Round DataTable 연동
8. UI 연동
9. Boss 라운드 구현
10. 이펙트/사운드/밸런싱
