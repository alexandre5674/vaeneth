// Copyright Vaeneth. All Rights Reserved.

#include "CombatSubsystem.h"

void UCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem initialized"));
}

void UCombatSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem deinitialized"));
    Super::Deinitialize();
}
