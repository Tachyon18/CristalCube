// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "CristalCubeStruct.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFadeComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCubeTransition, FIntPoint, NewCoordinate);

class UNiagaraSystem;

//==============================================================================
// UPGRADE SYSTEM (Simplified DataTable Approach)
//==============================================================================

// Simple Upgrade Types
UENUM(BlueprintType)
enum class EUpgradeType : uint8
{
    None            UMETA(DisplayName = "None"),
    Damage          UMETA(DisplayName = "Damage"),
    AttackSpeed     UMETA(DisplayName = "Attack Speed"),
    MoveSpeed       UMETA(DisplayName = "Move Speed"),
    Health          UMETA(DisplayName = "Health"),

};

//====================================================================================
// Attack Hit Types
//====================================================================================

UENUM(BlueprintType)
enum class EAttackHitType : uint8
{
	Point       UMETA(DisplayName = "Point"),       // one target
	Sphere      UMETA(DisplayName = "Sphere"),      // circular area 360 degree
    Line        UMETA(DisplayName = "Line"),        // 
    Box         UMETA(DisplayName = "Box"),         // square
    Cone        UMETA(DisplayName = "Cone"),        // 
    Capsule     UMETA(DisplayName = "Capsule")      // 
};

//==============================================================================
// CUBE MANAGEMENT SYSTEM
//==============================================================================

UENUM(BlueprintType)
enum class EBoundaryDirection : uint8
{
    Right UMETA(DisplayName = "Right"),
    Left UMETA(DisplayName = "Left"),
    Up UMETA(DisplayName = "Up"),
    Down UMETA(DisplayName = "Down")
};


UENUM(BlueprintType)
enum class ECubeState : uint8
{
    Active UMETA(DisplayName = "Active"),      // ���� Ȱ�� ť��
    Frozen UMETA(DisplayName = "Frozen"),      // ��Ȱ�� (�ð� ����)
    Unloaded UMETA(DisplayName = "Unloaded")   // �޸𸮿� ����
};

//==============================================================================
// ENEMY BEHAVIOR SYSTEM
// ==============================================================================

UENUM(BlueprintType)
enum class EMovementBehavior : uint8
{
    Direct    UMETA(DisplayName = "Direct"),     // 직선 추적 (현재 방식)
    Step      UMETA(DisplayName = "Step"),       // N거리 이동 후 정지 반복
    Teleport  UMETA(DisplayName = "Teleport"),   // 타이머 기반 순간이동
    Waypoint  UMETA(DisplayName = "Waypoint"),   // 경유점 순환 (후순위, stub)
};

UENUM(BlueprintType)
enum class EStepPattern : uint8
{
    TrackPlayer UMETA(DisplayName = "Track Player"),  // Step마다 플레이어 위치 재조준
    Orbit       UMETA(DisplayName = "Orbit"),         // 명칭 재검토 대상 — 목표 지점은 원 위에 찍히지만
                                                      // 지점 간 실제 이동은 직선(Step 구조 특성상 매끄러운 호 불가).
                                                      // "회전"보다는 "원주 위 산발 이동"에 가까움. 이름/분류 재논의 예정.
};

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    Moving    UMETA(DisplayName = "Moving"),
    Attacking UMETA(DisplayName = "Attacking"),
};

UENUM(BlueprintType)
enum class EEnemyShapeType : uint8
{
    Cube        UMETA(DisplayName = "Cube"),
    Sphere      UMETA(DisplayName = "Sphere"),
    Cylinder    UMETA(DisplayName = "Cylinder"),
    Cone        UMETA(DisplayName = "Cone"),
    Tetrahedron UMETA(DisplayName = "Tetrahedron"),  // Blender 에셋 — 후순위
    Octahedron  UMETA(DisplayName = "Octahedron"),   // Blender 에셋 — 후순위
    Custom      UMETA(DisplayName = "Custom"),        // 직접 메시 지정
};

//==============================================================================
// STRUCTS
//==============================================================================


// DataTable Row Structure
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeUpgradeDataRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    //FCristalCubeUpgradeDataRow();

    // Basic Info (for UI)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    FString Description;

    // Upgrade Logic
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    EUpgradeType UpgradeType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    float Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    int32 Weight;                   // ���� Ȯ�� ����ġ
};

//==============================================================================
// WEAPON SYSTEM STRUCTS
//==============================================================================

// Weapon Categories
UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
    None        UMETA(DisplayName = "None"),
    Melee       UMETA(DisplayName = "Melee"),        
    Ranged      UMETA(DisplayName = "Ranged"),       
    Magic       UMETA(DisplayName = "Magic")         
};

// Weapon Target Types
UENUM(BlueprintType)
enum class ETargetingMode : uint8
{
	None       UMETA(DisplayName = "No Targeting"),
    Single     UMETA(DisplayName = "Single Target"),
    Multi      UMETA(DisplayName = "Multi Target"),
    Area       UMETA(DisplayName = "Area(All in Range)"),
    Self       UMETA(DisplayName = "Self")
};

/// <Skill System>
/// 새로 작성된 모듈형 스킬 시스템 구조체, 프로토타입 버전

//==============================================================================
// MODULAR SKILL SYSTEM (Week 9 - Simplified Prototype)
//==============================================================================

// Forward declarations
class UNiagaraSystem;

//------------------------------------------------------------------------------
// Core Types - 스킬의 기본 형태 (5개, 지속적 추가)
//------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ESkillCoreType : uint8
{
    None            UMETA(DisplayName = "None"),
    Projectile      UMETA(DisplayName = "Projectile"),      // 투사체 (날아감)
    Instant         UMETA(DisplayName = "Instant"),         // 즉발 (히트스캔)
    Area            UMETA(DisplayName = "Area"),            // 범위 (바닥/공간)
    Beam            UMETA(DisplayName = "Beam"),            // 레이저 
	Rainfall 	    UMETA(DisplayName = "Rainfall")         // 낙하 (위에서 떨어짐)

};

UENUM(BlueprintType)
enum class EDropPattern : uint8
{
    Random  UMETA(DisplayName = "Random"),   // 반경 내 랜덤 분포
    Ring    UMETA(DisplayName = "Ring"),     // 원형 균등 분포
    Grid    UMETA(DisplayName = "Grid"),     // 격자형 분포
    Spiral  UMETA(DisplayName = "Spiral"),   // 나선형 (예약)
    Wave    UMETA(DisplayName = "Wave")      // 안에서 밖으로 파도형 (예약)
};

USTRUCT(BlueprintType)
struct FRainfallCoreData
{
    GENERATED_BODY()

    // 스폰 높이: TargetLocation 기준 Z 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall")
    float SpawnHeight = 800.0f;

    // 낙하 투사체 수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall",
        meta = (ClampMin = "1"))
    int32 DropCount = 8;

    // 투사체 간 스폰 간격 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall",
        meta = (ClampMin = "0.0"))
    float DropInterval = 0.08f;

    // 목표 영역 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall",
        meta = (ClampMin = "0.0"))
    float AreaRadius = 350.0f;

    // 낙하 패턴
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall")
    EDropPattern DropPattern = EDropPattern::Random;

    // 착지 예정 위치 경고 마커 표시 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall")
    bool bShowWarningIndicator = true;

    // 경고 마커 표시 시간 (투사체 스폰 전 대기)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall",
        meta = (ClampMin = "0.0"))
    float WarningDuration = 0.4f;

    // 착지 경고 이펙트 (NS_Rainfall_Warning)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainfall")
    UNiagaraSystem* WarningEffect = nullptr;
};

//------------------------------------------------------------------------------
// Addon Types - 추가 효과 (4개, 지속적 추가)
//------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ESkillAddonType : uint8
{
    None            UMETA(DisplayName = "None"),
    Explosion       UMETA(DisplayName = "Explosion"),      // 충돌 시 폭발
    Chain           UMETA(DisplayName = "Chain"),          // 다음 적으로 연쇄
    Penetrate       UMETA(DisplayName = "Penetrate"),      // 관통
    MultiShot       UMETA(DisplayName = "MultiShot")       // 다중 발사
};

//------------------------------------------------------------------------------
// Addon-specific Data Structs (Week 9 - Phase 3.5)
// 각 Addon의 수치를 독립된 구조체로 관리. 하드코딩 제거 목적.
//------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FExplosionAddonData
{
    GENERATED_BODY()

    // 폭발 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Explosion")
    float Radius = 300.0f;

    // 외곽 최소 데미지 비율 (중심 100%, 외곽 MinRatio% / 이후 추가 작업) 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Explosion",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinDamageRatio = 0.5f;

    // 폭발 전용 VFX — SkillDefinition에서 분리, 폭발 Addon이 책임짐
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Explosion")
    UNiagaraSystem* ExplosionEffect = nullptr;
};

USTRUCT(BlueprintType)
struct FChainAddonData
{
    GENERATED_BODY()

    // 최대 연쇄 횟수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Chain")
    int32 ChainCount = 3;

    // 연쇄당 데미지 감쇠율 (0.8 = 80% 유지)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Chain",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DamageDecay = 0.8f;

    // 다음 타겟 탐색 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Chain")
    float SearchRadius = 600.0f;

    // 연쇄 연결선 VFX (번개줄기, 사슬 등) — 연쇄 Addon이 책임짐
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Chain")
    UNiagaraSystem* ChainEffect = nullptr;
};

USTRUCT(BlueprintType)
struct FPenetrateAddonData
{
    GENERATED_BODY()

    // 최대 관통 횟수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetrate")
    int32 PierceCount = 3;          // 최대 관통 수

    // true: 관통 시 CurrentDamage 유지, false: 매번 BaseDamage 고정 (이후 추가 작업)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Penetrate")
    bool bInheritDamage = true;
};

USTRUCT(BlueprintType)
struct FMultiShotAddonData
{
    GENERATED_BODY()

    // 추가 발사체 수 (총 발사 수 = ProjectileCount + AdditionalCount)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|MultiShot",
        meta = (ClampMin = "1"))
    int32 AdditionalCount = 2;

    // 전체 확산 각도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|MultiShot",
        meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float SpreadAngle = 60.0f;
};

//------------------------------------------------------------------------------
// Element Types - 원소 속성
//------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ESkillElementType : uint8
{
    None            UMETA(DisplayName = "None"),
    Physical        UMETA(DisplayName = "Physical"),       // 물리
    Fire            UMETA(DisplayName = "Fire"),           // 화염
    Ice             UMETA(DisplayName = "Ice"),            // 얼음
    Lightning       UMETA(DisplayName = "Lightning"),      // 번개
    Poison          UMETA(DisplayName = "Poison")          // 독
};

//------------------------------------------------------------------------------
// Passive Properties - 수치 강화
//------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FSkillPassiveProperties
{
    GENERATED_BODY()

    // 배율 (곱셈)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Multipliers")
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Multipliers")
    float SizeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Multipliers")
    float SpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Counts")
    int32 ProjectileCount = 1;

    // Addon별 상세 데이터 (해당 Addon 미사용 시에도 기본값 유지)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Addon Data")
    FPenetrateAddonData PierceData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Addon Data")
    FChainAddonData ChainData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Addon Data")
    FExplosionAddonData ExplosionData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive|Addon Data")
    FMultiShotAddonData MultiShotData;
};

//------------------------------------------------------------------------------
// Skill Definition - 완전한 스킬 정의
//------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FSkillDefinition
{
    GENERATED_BODY()

    // === 기본 정보 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float BaseDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float Cooldown = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float Range = 1000.0f;

    // === Core (필수, 하나만) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
    ESkillCoreType CoreType = ESkillCoreType::Projectile;

    // === Addons (선택, 여러 개) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addons")
    TArray<ESkillAddonType> Addons;

    // === Passive (수치) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    FSkillPassiveProperties Passives;

    // === Element (비주얼/효과) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    ESkillElementType ElementType = ESkillElementType::Physical;

    // === VFX ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UNiagaraSystem* SkillEffect = nullptr;   // 스킬 주 비주얼 (Core가 운용)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UNiagaraSystem* ImpactEffect = nullptr;  // 타격 피드백 (속성 엔진 연동 대상)

    // Projectile Core 전용. VFX 포함 BP 서브클래스를 에디터에서 직접 지정.
    // null이면 SkillSystem의 기본 SkillEffectorClass로 폴백.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
    TSubclassOf<class ACC_SkillEffector> ProjectileClass;

    // Rainfall Core 전용 데이터
    // EditCondition으로 Rainfall일 때만 에디터에 노출
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core",
        meta = (EditCondition = "CoreType == ESkillCoreType::Rainfall", EditConditionHides))
    FRainfallCoreData RainfallData;

    // === Audio ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* CastSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* HitSound = nullptr;
};

//------------------------------------------------------------------------------
// Skill Execution Context - 런타임 데이터
//------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FSkillExecutionContext
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Caster = nullptr;

    UPROPERTY()
    FVector StartLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector Direction = FVector::ForwardVector;

    // 런타임 추적
    UPROPERTY()
    TArray<AActor*> HitActors;          // 이미 맞은 적들 (관통/연쇄용)

    UPROPERTY()
    int32 CurrentChainCount = 0;

    UPROPERTY()
    int32 CurrentPierceCount = 0;

    UPROPERTY()
    float CurrentDamage = 0.0f;
};

//------------------------------------------------------------------------------
// DataTable Row - 스킬 라이브러리
//------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FSkillTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FSkillDefinition SkillData;

    // UI 전용
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FText Description;

    // 드롭/획득 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquisition")
    float DropWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquisition")
    bool bIsStartingSkill = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquisition")
    int32 UnlockLevel = 1;
};

///==============================================================================
/// 속성별 색상 정보
/// ==============================================================================

USTRUCT(BlueprintType)
struct FElementColorData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor PrimaryColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor SecondaryColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EmissiveStrength = 1.0f;
};


/// </Skill System>

//==============================================================================
// CUBE CLEAR REWARD SYSTEM
//==============================================================================

UENUM(BlueprintType)
enum class ECubeClearRewardType : uint8
{
    WeaponUpgrade   UMETA(DisplayName = "Weapon Upgrade"),   // 무기 강화
    SkillGrant      UMETA(DisplayName = "Skill Grant"),      // 스킬 획득
    StatBoost       UMETA(DisplayName = "Stat Boost"),       // 스탯 부스트 (영구)
    HealFull        UMETA(DisplayName = "Full Heal"),        // 체력 완전 회복
};

USTRUCT(BlueprintType)
struct FCubeClearReward
{
    GENERATED_BODY()

    /** 보상 종류 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
    ECubeClearRewardType RewardType = ECubeClearRewardType::WeaponUpgrade;

    /** UI 표시 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
    FText DisplayName;

    /** UI 표시 설명 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
    FText Description;

    /** 아이콘 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
    UTexture2D* Icon = nullptr;

    /** StatBoost 용 — 강화 타입 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Stat")
    EUpgradeType StatUpgradeType = EUpgradeType::None;

    /** StatBoost 용 — 강화 수치 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Stat")
    float StatValue = 0.0f;

    /** WeaponUpgrade / SkillGrant 용 — DataTable 행 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Data")
    FName DataRowName = NAME_None;
};

// Bast Weapon Data
USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Info")
    FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Info")
	FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Info")
	UTexture2D* Icon;
    
    UPROPERTY(Editanywhere, BlueprintReadOnly)
	TSubclassOf<class ACC_Weapon> WeaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxLevel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsStartingWeapon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float DropWeight;

    FWeaponData()
        : WeaponName(FText::FromString("Unknown Weapon"))
        , Description(FText::FromString("No description"))
        , Icon(nullptr)
        , WeaponClass(nullptr)
        , MaxLevel(5)
        , bIsStartingWeapon(true)
        , DropWeight(1.0f)
    {
    }
};

// Base Weapon Stats Data
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeWeaponStats
{
    GENERATED_BODY()

public:
    //FCristalCubeWeaponStats();

    // Basic Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Basic")
    EWeaponCategory WeaponCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Basic")
    FString WeaponName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Basic")
    float BaseDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Basic")
    float AttackSpeed;

    FCristalCubeWeaponStats()
        : WeaponCategory(EWeaponCategory::None)
        , BaseDamage(10.0f)
        , AttackSpeed(1.0f)
    {
        // Default weapon stats
        // - BaseDamage: 10 damage per hit
        // - AttackSpeed: 1 attack per second
    }
};

// Melee Weapon Specific Stats
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeMeleeStats
{
    GENERATED_BODY()

public:
    //FCristalCubeMeleeStats();

    // Melee-specific stats (3-4 key stats)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    float AttackRange;          // ���� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    float SwingAngle;           // �ֵθ��� ���� (��)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    int32 ComboCount;           // ���� ���� Ƚ��

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    float KnockbackForce;       // �˹� ����
};

// Ranged Weapon Specific Stats  
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeRangedStats
{
    GENERATED_BODY()

public:
    //FCristalCubeRangedStats();

    // Ranged-specific stats (3-4 key stats)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
    float ProjectileSpeed;      // ����ü �ӵ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
    float MaxRange;             // �ִ� ��Ÿ�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
    int32 ProjectileCount;      // ���� �߻� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
    float Accuracy;             // ��Ȯ�� (0.0~1.0)
};

// Magic Weapon Specific Stats
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeMagicStats
{
    GENERATED_BODY()

public:
    //FCristalCubeMagicStats();

    // Magic-specific stats (3-4 key stats)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic")
    float CastTime;             // ��â �ð�

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic")
    float EffectRadius;         // ���� ȿ�� ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic")
    float Duration;             // ���ӽð� (����/�������)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic")
    float ManaCost;             // ���� �Ҹ�
};

//==============================================================================
// CHARACTER STATS SYSTEM
//==============================================================================

// Basic Stats - Essential for Week 3-4 MVP
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeBasicStats
{
    GENERATED_BODY()

    // Essential stats for MVP (Week 3-4)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Essential")
    float DamageMultiplier;          // 1.0 = base, 1.2 = +20% damage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Essential")
    float AttackSpeedMultiplier;     // 1.0 = base, 1.5 = +50% attack speed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Essential")
    float MoveSpeedMultiplier;       // 1.0 = base, 1.3 = +30% move speed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Essential")
    float HealthMultiplier;          // 1.0 = base, 1.5 = +50% health

    FCristalCubeBasicStats()
        : DamageMultiplier(1.0f)
        , AttackSpeedMultiplier(1.0f)
        , MoveSpeedMultiplier(1.0f)
        , HealthMultiplier(1.0f)
    {
    }
};

// Advanced Stats - For future expansion (Week 5+)
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeAdvancedStats
{
    GENERATED_BODY()

    // Advanced combat mechanics (not used in MVP)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float CriticalChance;            // Critical hit chance (0.0~1.0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float CriticalDamageMultiplier;  // Critical damage multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float AreaOfEffectMultiplier;    // AoE size multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    int32 ProjectileCountBonus;      // Extra projectiles

    FCristalCubeAdvancedStats()
        : CriticalChance(0.0f)
        , CriticalDamageMultiplier(1.5f)
        , AreaOfEffectMultiplier(1.0f)
        , ProjectileCountBonus(0)
    {
    }
};

// Defense Stats - For future expansion (Week 5+)
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeDefenseStats
{
    GENERATED_BODY()

    // Defensive mechanics (not used in MVP)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float DamageReduction;           // Damage reduction (0.0~1.0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float Evasion;                   // Evasion chance (0.0~1.0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float HealthRegeneration;        // HP regen per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float Shield;                    // Shield amount

    FCristalCubeDefenseStats()
        : DamageReduction(0.0f)
        , Evasion(0.0f)
        , HealthRegeneration(0.0f)
        , Shield(0.0f)
    {
    }
};

// Utility Stats - For future expansion (Week 5+)
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeUtilityStats
{
    GENERATED_BODY()

    // Utility mechanics (not used in MVP)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float ExperienceMultiplier;      // XP gain multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float PickupRange;               // Pickup range multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float CooldownReduction;         // Cooldown reduction (0.0~1.0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Future")
    float LuckBonus;                 // Luck bonus

    FCristalCubeUtilityStats()
        : ExperienceMultiplier(1.0f)
        , PickupRange(1.0f)
        , CooldownReduction(0.0f)
        , LuckBonus(0.0f)
    {
    }
};

// Player Stats - Main container combining all stat categories
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubePlayerStats
{
    GENERATED_BODY()

    // Essential stats (Week 3-4)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FCristalCubeBasicStats BasicStats;

    // Future expansion stats (Week 5+)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FCristalCubeAdvancedStats AdvancedStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FCristalCubeDefenseStats DefenseStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FCristalCubeUtilityStats UtilityStats;

    FCristalCubePlayerStats()
        : BasicStats()
        , AdvancedStats()
        , DefenseStats()
        , UtilityStats()
    {
    }
};

USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeEnemyStats
{
    GENERATED_BODY()

	// Enemy stats (simplified for MVP)

public:

    UPROPERTY(EditAnywhere, BlueprintREadWrite, Category = "Stats")
    float AttackDamage;

	UPROPERTY(EditAnywhere, BlueprintREadWrite, Category = "Stats")
    float AttackCooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRange; 

    FCristalCubeEnemyStats()
    {
        AttackDamage = 10.0f;
        AttackCooldown = 1.0f;
        AttackRange = 200.0f;
    }
};

//==============================================================================
// CRYSTAL COLLECTION SYSTEM
//==============================================================================

// Crystal Collection Data
USTRUCT(BlueprintType)
struct CRISTALCUBE_API FCristalCubeCollectionData
{
    GENERATED_BODY()

public:
    //FCristalCubeCollectionData();

    // Collection Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Basic")
    int32 TotalCristalsCollected;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Basic")
    float CollectionRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Basic")
    bool bAutoCollect;

    // Experience System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Experience")
    float ExperienceMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Experience")
    float BonusExperienceChance;

    // Magnetic Effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Magnetic")
    bool bMagneticCollection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection|Magnetic")
    float MagneticStrength;
};

//==============================================================================
// Attack Hit Data
//==============================================================================

USTRUCT(BlueprintType)
struct FAttackHitData
{
    GENERATED_BODY()

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    EAttackHitType HitType = EAttackHitType::Line;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float Range = 200.0f;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float Width = 100.0f;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float Height = 25.0f;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float Thickness = 50.0f;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0", ClampMax = "360"))
    float Angle = 90.0f;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float Radius = 50.0f;

    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    bool bCheckFrontOnly = false;

    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    bool bPenetrate = false;

    FAttackHitData()
        : HitType(EAttackHitType::Sphere)
        , Range(200.0f)
        , Width(100.0f)
        , Height(25.0f)
        , Thickness(50.0f)
        , Angle(90.0f)
        , Radius(50.0f)
        , bCheckFrontOnly(false)
        , bPenetrate(false)
    {
    }
};

//==============================================================================
// Actor Save Data
//==============================================================================

USTRUCT(BlueprintType)
struct FActorSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<AActor> ActorClass;

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    TMap<FString, FString> ActorState;
};

//==============================================================================
// Cube Data
//==============================================================================

UENUM(BlueprintType)
enum class ECubeTheme : uint8
{
    None        UMETA(DisplayName = "None"),
    Grassland   UMETA(DisplayName = "Grassland"),   // 초원
    Desert      UMETA(DisplayName = "Desert"),       // 사막
    Volcano     UMETA(DisplayName = "Volcano"),      // 화산
    Snowfield   UMETA(DisplayName = "Snowfield"),    // 설원
    Swamp       UMETA(DisplayName = "Swamp"),        // 늪지
    Cave        UMETA(DisplayName = "Cave"),         // 동굴
    Ruin        UMETA(DisplayName = "Ruin")          // 폐허
};

USTRUCT(BlueprintType)
struct FCubeThemeData
{
    GENERATED_BODY()

    /** FloorMesh에 적용할 머티리얼 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Material")
    UMaterialInterface* FloorMaterial = nullptr;

    /** World 공간 기준 텍스처 타일 크기 (cm) — 큐브 크기 무관 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Material",
        meta = (ClampMin = "50.0"))
    float TextureTileSize = 200.0f;

    /** 테마 주 색상 (머티리얼 파라미터: ThemeColor) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Color")
    FLinearColor ThemeColor = FLinearColor(0.70f, 0.80f, 1.00f, 1.f);

    /** 테마 보조 색상 (머티리얼 파라미터: ThemeColorSecondary) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Color")
    FLinearColor ThemeColorSecondary = FLinearColor(1.00f, 1.00f, 1.00f, 1.f);

    /** 이미시브 강도 (머티리얼 파라미터: EmissiveStrength) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Color",
        meta = (ClampMin = "0.0"))
    float EmissiveStrength = 1.0f;

    // ─── Scatter 설정 ───────────────────────────────────────

    /** 이 테마에서 HISM으로 배치할 스태틱 메시 목록 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Scatter")
    TArray<UStaticMesh*> ScatterMeshes;

    /** 메시별 가중치 (ScatterMeshes와 인덱스 일치, 비어있으면 균등 분배) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Scatter")
    TArray<int32> ScatterMeshWeights;

    /** 스캐터 최소 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Scatter",
        meta = (ClampMin = "0"))
    int32 ScatterMinCount = 15;

    /** 스캐터 최대 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme|Scatter",
        meta = (ClampMin = "0"))
    int32 ScatterMaxCount = 35;
};

USTRUCT(BlueprintType)
struct FCubeData
{
    GENERATED_BODY()

    UPROPERTY()
    FIntPoint Coordinate = FIntPoint(0, 0);

    UPROPERTY()
    TArray<FActorSaveData> SavedActors;

    UPROPERTY()
    ECubeState State = ECubeState::Unloaded;

    UPROPERTY()
    bool bCleared = false;

    UPROPERTY()
    ECubeTheme CubeType = ECubeTheme::None; //
};

//==============================================================================
// GLASS UI SYSTEM
//==============================================================================

UENUM(BlueprintType)
enum class EGlassTheme : uint8
{
    Ocean   UMETA(DisplayName = "Ocean"),    // (0.20, 0.60, 1.00)
    Galaxy  UMETA(DisplayName = "Galaxy"),   // (0.55, 0.25, 1.00)
    Sunset  UMETA(DisplayName = "Sunset"),   // (1.00, 0.30, 0.55)
    Fire    UMETA(DisplayName = "Fire"),     // (1.00, 0.45, 0.10)
    Forest  UMETA(DisplayName = "Forest"),   // (0.15, 0.85, 0.55)
    White   UMETA(DisplayName = "White"),    // (0.70, 0.80, 1.00)
    Custom  UMETA(DisplayName = "Custom")    // 직접 지정
};

USTRUCT(BlueprintType)
struct FGlassThemeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Panel")
    FLinearColor TintColor = FLinearColor(0.08f, 0.25f, 0.80f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Panel")
    float PanelOpacity = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Panel")
    float PrismIntensity = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Glow")
    FLinearColor GlowColor = FLinearColor(0.20f, 0.60f, 1.00f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Glow")
    float GlowOpacity = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Shape")
    float CornerRadius = 0.08f;  // UV 비율

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Shape")
    float AspectRatio = 2.0f;   // Width / Height

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Blur")
    float BlurStrength = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BeamOffset = 0.40f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BeamWidth = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BeamSoftness = 0.08f;

    // ── 통합 광원 파라미터 (v3) ────────────────────────────────
    // XY = UV위치, Z = 반경, W = 미사용(0)
    // SetVectorParameterValue에 직접 전달 가능 (FLinearColor 일치)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light0_Color = FLinearColor(0.55f, 0.25f, 1.00f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    float Light0_Intensity = 0.24f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light1_PosRadius = FLinearColor(0.82f, 0.15f, 0.30f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light1_Color = FLinearColor(0.80f, 0.45f, 1.00f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    float Light1_Intensity = 0.48f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light2_PosRadius = FLinearColor(0.15f, 0.80f, 0.34f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light2_Color = FLinearColor(0.30f, 0.18f, 1.00f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    float Light2_Intensity = 0.36f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light3_PosRadius = FLinearColor(0.55f, 0.50f, 0.14f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    FLinearColor Light3_Color = FLinearColor(1.00f, 0.45f, 0.90f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    float Light3_Intensity = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass|Lights")
    float SceneLightsOpacityBoost = 0.35f;
};



UCLASS()
class CRISTALCUBE_API ACristalCubeStruct : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACristalCubeStruct();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
