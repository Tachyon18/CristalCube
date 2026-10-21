// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_LogHelper.h"

FString UCC_LogHelper::GetCleanActorName(const AActor* Actor)
{
    if (!Actor)
    {
        return TEXT("NULL");
    }

    // 클래스 이름 정리
    FString ClassName = Actor->GetClass()->GetName();

    // Blueprint 접두사/접미사 제거
    ClassName = ClassName.Replace(TEXT("BP_"), TEXT(""));
    ClassName = ClassName.Replace(TEXT("_C"), TEXT(""));

    // CristalCube 접두사도 제거해서 더 짧게
    ClassName = ClassName.Replace(TEXT("CristalCube"), TEXT(""));
    ClassName = ClassName.Replace(TEXT("CC_"), TEXT(""));

    // 고유 ID 추가 (마지막 3자리만 사용)
    int32 UniqueID = Actor->GetUniqueID() % 1000;

    return FString::Printf(TEXT("%s_%d"), *ClassName, UniqueID);
}

FString UCC_LogHelper::GetCleanClassName(const UObject* Object)
{
    if (!Object)
    {
        return TEXT("NULL");
    }

    FString ClassName = Object->GetClass()->GetName();

    // 공통 접두사/접미사 제거
    ClassName = ClassName.Replace(TEXT("BP_"), TEXT(""));
    ClassName = ClassName.Replace(TEXT("_C"), TEXT(""));
    ClassName = ClassName.Replace(TEXT("CristalCube"), TEXT(""));
    ClassName = ClassName.Replace(TEXT("CC_"), TEXT(""));

    return ClassName;
}

FColor UCC_LogHelper::GetLogColor(const FString& InLogType)
{
    // 로그 타입별 컬러 설정
    if (InLogType == TEXT("PLAYER"))
    {
        return FColor::Cyan;
    }
    else if (InLogType == TEXT("ENEMY"))
    {
        return FColor::Red;
    }
    else if (InLogType == TEXT("AI"))
    {
        return FColor::Blue;
    }
    else if (InLogType == TEXT("COMBAT"))
    {
        return FColor::Orange;
    }
    else if (InLogType == TEXT("WEAPON"))
    {
        return FColor::Magenta;
    }
    else if (InLogType == TEXT("STATS"))
    {
        return FColor::Yellow;
    }
    else
    {
        return FColor::White;  // 기본 색상
    }
}
