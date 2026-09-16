// Copyright Vaeneth. All Rights Reserved.

#include "FractureSubsystem.h"

void UFractureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("FractureSubsystem initialized"));
}

void UFractureSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("FractureSubsystem deinitialized"));
    Super::Deinitialize();
}
