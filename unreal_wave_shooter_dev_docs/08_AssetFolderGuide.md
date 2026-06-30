# 08. Asset Folder Guide

## 1. Content 폴더 구조

```text
Content
├─ CWS
│  ├─ Blueprints
│  │  ├─ Player
│  │  ├─ Enemy
│  │  ├─ Boss
│  │  ├─ Weapon
│  │  ├─ Wave
│  │  ├─ Spawn
│  │  ├─ Pickup
│  │  └─ UI
│  ├─ Maps
│  ├─ Materials
│  ├─ Meshes
│  │  ├─ Characters
│  │  ├─ Enemies
│  │  ├─ Props
│  │  └─ Environment
│  ├─ Animations
│  ├─ VFX
│  ├─ Audio
│  ├─ Data
│  │  ├─ DataTables
│  │  └─ Curves
│  └─ UI
│     ├─ Widgets
│     ├─ Icons
│     └─ Fonts
```

## 2. Source 폴더 구조

```text
Source
└─ CentralWaveShooter
   ├─ Public
   │  ├─ Character
   │  ├─ Components
   │  ├─ Enemy
   │  ├─ Weapon
   │  ├─ Wave
   │  ├─ Spawn
   │  ├─ Pickup
   │  ├─ UI
   │  └─ Data
   └─ Private
      ├─ Character
      ├─ Components
      ├─ Enemy
      ├─ Weapon
      ├─ Wave
      ├─ Spawn
      ├─ Pickup
      ├─ UI
      └─ Data
```

## 3. 무료 리소스 조합

| 용도 | 추천 출처 |
|---|---|
| 맵/환경 | Fab, Quixel Megascans |
| 캐릭터/애니메이션 | Mixamo |
| UI/아이콘 | Kenney |
| 보조 소품 | Poly Pizza, itch.io |
| 효과음/음악 | OpenGameArt, itch.io |

## 4. 에셋 관리 규칙

- 외부 에셋은 바로 사용하지 말고 `External` 또는 `Downloaded` 폴더에 먼저 보관한다.
- 최종 사용 에셋만 프로젝트 구조에 복사한다.
- 라이선스 파일이 있으면 함께 보관한다.
- 파일명에 공백과 한글을 피한다.
- Material Instance를 만들어 색상/거칠기만 조정해서 스타일을 통일한다.

## 5. 추천 네이밍

```text
BP_PlayerCharacter
BP_Enemy_Normal
BP_Enemy_Fast
BP_Enemy_Tank
BP_Boss
BP_SpawnPoint_North
BP_WaveManager
WBP_HUD
DT_RoundData
DT_EnemyData
M_Concrete_Floor
MI_Concrete_Floor_Arena
```
