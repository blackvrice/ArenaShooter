# 04. DataTables And Structs

## 1. Enum 설계

```cpp
UENUM(BlueprintType)
enum class ECWSSpawnDirection : uint8
{
    North       UMETA(DisplayName = "북"),
    South       UMETA(DisplayName = "남"),
    East        UMETA(DisplayName = "동"),
    West        UMETA(DisplayName = "서"),
    NorthEast   UMETA(DisplayName = "북동"),
    NorthWest   UMETA(DisplayName = "북서"),
    SouthEast   UMETA(DisplayName = "남동"),
    SouthWest   UMETA(DisplayName = "남서"),
    Center      UMETA(DisplayName = "중앙")
};
```

```cpp
UENUM(BlueprintType)
enum class ECWSEnemyType : uint8
{
    Normal,
    Fast,
    Tank,
    Boss
};
```

## 2. Enemy Data Row

```cpp
USTRUCT(BlueprintType)
struct FCWSEnemyDataRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECWSEnemyType EnemyType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<class ACWSEnemyBase> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MaxHealth = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MoveSpeed = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float AttackDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ScoreValue = 100;
};
```

## 3. Round Spawn Group

```cpp
USTRUCT(BlueprintType)
struct FCWSRoundSpawnGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECWSSpawnDirection Direction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECWSEnemyType EnemyType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Count = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float SpawnInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bUseBossClass = false;
};
```

## 4. Round Definition

```cpp
USTRUCT(BlueprintType)
struct FCWSRoundDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 RoundNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCWSRoundSpawnGroup> SpawnGroups;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PreRoundDelay = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PostRoundDelay = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bBossRound = false;
};
```

## 5. CSV 예시 - Round Data

아래 값은 DataTable CSV로 변환하거나, Unreal Editor에서 직접 입력해도 된다.

```csv
Name,RoundNumber,bBossRound,PreRoundDelay,PostRoundDelay,Description
Round_01,1,false,3.0,5.0,"북/남 기본 웨이브"
Round_02,2,false,3.0,5.0,"동서남북 4방향 웨이브"
Round_03,3,false,3.0,5.0,"4방향 + 북동/남서"
Round_04,4,false,3.0,5.0,"8방향 전방위 웨이브"
Round_05,5,true,5.0,0.0,"중앙 보스 출현"
```

## 6. 라운드별 SpawnGroup 권장값

현재 C++ 구현은 아래 방향, EnemyType, Count를 기본값으로 사용한다. Normal/Fast/Tank/Boss는 각각 `ACWSEnemyBase`/`ACWSFastEnemy`/`ACWSTankEnemy`/`ACWSBossEnemy`로 해석된다. Round 5의 Center 한 개만 Boss 슬롯이다.

### Round 1

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Normal | 4 | 1.2 |
| South | Normal | 4 | 1.2 |

### Round 2

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Normal | 4 | 1.0 |
| South | Normal | 4 | 1.0 |
| East | Fast | 4 | 1.0 |
| West | Fast | 4 | 1.0 |

### Round 3

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Tank | 4 | 0.9 |
| South | Tank | 4 | 0.9 |
| East | Normal | 4 | 0.9 |
| West | Normal | 4 | 0.9 |
| NorthEast | Fast | 4 | 1.1 |
| SouthWest | Fast | 4 | 1.1 |

### Round 4

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Fast | 5 | 0.8 |
| South | Fast | 5 | 0.8 |
| East | Tank | 4 | 0.8 |
| West | Tank | 4 | 0.8 |
| NorthEast | Normal | 4 | 1.0 |
| NorthWest | Normal | 4 | 1.0 |
| SouthEast | Normal | 4 | 1.0 |
| SouthWest | Normal | 4 | 1.0 |

### Round 5

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| Center | Boss | 1 | 0.1 |
| North | Fast | 3 | 1.2 |
| South | Fast | 3 | 1.2 |
| East | Tank | 2 | 1.2 |
| West | Tank | 2 | 1.2 |
| NorthEast | Normal | 1 | 1.5 |
| NorthWest | Normal | 1 | 1.5 |
| SouthEast | Normal | 1 | 1.5 |
| SouthWest | Normal | 1 | 1.5 |
