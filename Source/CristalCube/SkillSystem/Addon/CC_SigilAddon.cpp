// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SigilAddon.h"
#include "../CC_SkillSystem.h"
#include "../CC_SigilEffector.h"

void UCC_SigilAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || !SkillSystem->GetWorld()) return;

    UWorld* World = SkillSystem->GetWorld();

    // Sigil은 바닥에 깔리는 장판형 Addon — 피격 지점의 Z(공중일 수 있음)를 그대로 쓰지 않고,
    // 같은 X/Y에서 아래로 트레이스해 실제 지면 높이를 찾아 그 위에 배치한다.
    // (평평한 세계를 가정한 고정 Z도 아니고, 피격 당시의 raw Z도 아닌 — 실제 3D 지형 기준)
    FVector GroundLocation = HitLocation;

    const FVector TraceStart(HitLocation.X, HitLocation.Y, HitLocation.Z + 500.0f);
    const FVector TraceEnd(HitLocation.X, HitLocation.Y, HitLocation.Z - 2000.0f);

    FHitResult GroundHit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SigilGroundTrace), false);
    if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        GroundLocation = GroundHit.ImpactPoint;
    }
    // 트레이스가 아무것도 못 찾으면 원래 HitLocation을 그대로 사용 (안전 폴백 — 지금까지의 동작과 동일)

    ACC_SigilEffector* Sigil = SkillSystem->GetWorld()->SpawnActor<ACC_SigilEffector>(HitLocation, FRotator::ZeroRotator);
    if (Sigil)
    {
        Sigil->Initialize(HitLocation, Context.Caster, Data, SkillSystem, Skill, Context, AddonIndex + 1);
    }
}

void UCC_SigilAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("SigilRadius"))
    {
        Data.Radius += ValuePerPoint;
    }
    else if (AttributeID == TEXT("SigilTickDamage"))
    {
        Data.TickDamage += ValuePerPoint;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SigilAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
