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
struct FCWSSpawnGroup
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
};
```

## 4. Round Data Row

```cpp
USTRUCT(BlueprintType)
struct FCWSRoundDataRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 RoundNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCWSSpawnGroup> SpawnGroups;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PreRoundDelay = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PostRoundDelay = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsBossRound = false;
};
```

## 5. CSV 예시 - Round Data

아래 값은 DataTable CSV로 변환하거나, Unreal Editor에서 직접 입력해도 된다.

```csv
Name,RoundNumber,bIsBossRound,PreRoundDelay,PostRoundDelay,Description
Round_01,1,false,3.0,5.0,"북/남 기본 웨이브"
Round_02,2,false,3.0,5.0,"동서남북 4방향 웨이브"
Round_03,3,false,3.0,5.0,"4방향 + 북동/남서"
Round_04,4,false,3.0,5.0,"8방향 전방위 웨이브"
Round_05,5,true,5.0,0.0,"중앙 보스 출현"
```

## 6. 라운드별 SpawnGroup 권장값

### Round 1

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Normal | 4 | 1.2 |
| South | Normal | 4 | 1.2 |

### Round 2

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Normal | 3 | 1.0 |
| South | Normal | 3 | 1.0 |
| East | Normal | 3 | 1.0 |
| West | Normal | 3 | 1.0 |
| East | Fast | 2 | 1.5 |
| West | Fast | 2 | 1.5 |

### Round 3

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Normal | 4 | 0.9 |
| South | Normal | 4 | 0.9 |
| East | Normal | 4 | 0.9 |
| West | Normal | 4 | 0.9 |
| NorthEast | Fast | 3 | 1.2 |
| SouthWest | Fast | 3 | 1.2 |
| NorthEast | Tank | 1 | 2.0 |
| SouthWest | Tank | 1 | 2.0 |

### Round 4

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| North | Normal | 3 | 0.8 |
| South | Normal | 3 | 0.8 |
| East | Normal | 4 | 0.8 |
| West | Normal | 4 | 0.8 |
| NorthEast | Fast | 2 | 1.0 |
| NorthWest | Fast | 2 | 1.0 |
| SouthEast | Fast | 2 | 1.0 |
| SouthWest | Fast | 2 | 1.0 |
| North | Tank | 1 | 2.5 |
| South | Tank | 1 | 2.5 |
| East | Tank | 1 | 2.5 |
| West | Tank | 1 | 2.5 |

### Round 5

| Direction | EnemyType | Count | Interval |
|---|---|---:|---:|
| Center | Boss | 1 | 0 |
| North | Normal | 2 | 1.5 |
| South | Normal | 2 | 1.5 |
| East | Fast | 2 | 2.0 |
| West | Fast | 2 | 2.0 |
| NorthEast | Tank | 1 | 4.0 |
| SouthWest | Tank | 1 | 4.0 |
