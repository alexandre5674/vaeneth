// Copyright Vaeneth. All Rights Reserved.

#include "FractureSubsystem.h"

#include "EventBusSubsystem.h"
#include "Save/SaveSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace FractureSubsystemConstants
{
    static const FGuid SaveIdentifier(0x8B5A7E4D, 0xA1C247B9, 0x91F0D5C2, 0x4E6A8B33);
    static const TCHAR* FractureClassPath = TEXT("/Game/Vaeneth/Systems/Fracture/BP_Fracture.BP_Fracture_C");
    static const TCHAR* BalanceCurveLibraryPath = TEXT("/Game/Vaeneth/Data/BPFL_BalanceCurves.BPFL_BalanceCurves_C");
    static const FName FractureRecordsKey(TEXT("FractureRecords"));
    static const FName FractureIDProperty(TEXT("FractureID"));
    static const FName GetFractureScaleFunction(TEXT("GetFractureScale"));
}

void UFractureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<USaveSubsystem>();

    if (USaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveSubsystem>())
    {
        SaveSubsystem->RegisterSaveable(this);
    }

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UFractureSubsystem::HandlePostLoadMap);
    UE_LOG(LogTemp, Log, TEXT("FractureSubsystem initialized and registered with SaveSubsystem"));
}

void UFractureSubsystem::Deinitialize()
{
    if (USaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveSubsystem>())
    {
        SaveSubsystem->UnregisterSaveable(this);
    }

    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    DestroySpawnedFractures();
    Fractures.Empty();
    Super::Deinitialize();
}

FGuid UFractureSubsystem::RegisterFracture(FFractureRecord Record)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("FractureSubsystem::RegisterFracture: World invalide."));
        return FGuid();
    }

    Record.UniqueID = FGuid::NewGuid();
    if (Record.LevelName.IsNone())
    {
        Record.LevelName = GetCurrentLevelName();
    }
    if (Record.CreationTimestamp <= 0.0f)
    {
        Record.CreationTimestamp = UGameplayStatics::GetTimeSeconds(World);
    }

    Fractures.Add(Record.UniqueID, Record);
    SpawnFractureActor(Record, true);
    return Record.UniqueID;
}

TArray<FFractureRecord> UFractureSubsystem::GetFracturesForLevel(FName Level) const
{
    TArray<FFractureRecord> Result;
    for (const TPair<FGuid, FFractureRecord>& Pair : Fractures)
    {
        if (Pair.Value.LevelName == Level)
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}

bool UFractureSubsystem::SealFracture(FGuid ID)
{
    if (FFractureRecord* Record = Fractures.Find(ID))
    {
        Record->bSealed = true;
        return true;
    }
    return false;
}

void UFractureSubsystem::SpawnFracturesForCurrentLevel()
{
    const FName CurrentLevel = GetCurrentLevelName();
    if (CurrentLevel.IsNone())
    {
        return;
    }

    for (const TPair<FGuid, FFractureRecord>& Pair : Fractures)
    {
        const FFractureRecord& Record = Pair.Value;
        if (Record.LevelName == CurrentLevel)
        {
            SpawnFractureActor(Record, false);
        }
    }
}

int32 UFractureSubsystem::GetFractureCount() const
{
    return Fractures.Num();
}

FSaveBlob UFractureSubsystem::GatherSaveData_Implementation()
{
    FFractureSavePayload Payload;
    Fractures.GenerateValueArray(Payload.Records);

    FSaveBlob Blob;
    Blob.Strings.Add(FractureSubsystemConstants::FractureRecordsKey, FString());
    if (!FJsonObjectConverter::UStructToJsonObjectString(Payload, Blob.Strings[FractureSubsystemConstants::FractureRecordsKey]))
    {
        UE_LOG(LogTemp, Error, TEXT("FractureSubsystem: échec de sérialisation des FractureRecords."));
        Blob.Strings.Remove(FractureSubsystemConstants::FractureRecordsKey);
    }
    return Blob;
}

bool UFractureSubsystem::ApplySaveData_Implementation(const FSaveBlob& Data)
{
    const FString* Json = Data.Strings.Find(FractureSubsystemConstants::FractureRecordsKey);
    if (!Json || Json->IsEmpty())
    {
        return false;
    }

    FFractureSavePayload Payload;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(*Json, &Payload, 0, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("FractureSubsystem: échec de désérialisation des FractureRecords."));
        return false;
    }

    DestroySpawnedFractures();
    Fractures.Empty();
    for (const FFractureRecord& Record : Payload.Records)
    {
        if (Record.UniqueID.IsValid())
        {
            Fractures.Add(Record.UniqueID, Record);
        }
    }

    SpawnFracturesForCurrentLevel();
    return true;
}

FGuid UFractureSubsystem::GetSaveIdentifier_Implementation()
{
    return FractureSubsystemConstants::SaveIdentifier;
}

void UFractureSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (LoadedWorld == GetWorld())
    {
        SpawnFracturesForCurrentLevel();
    }
}

AActor* UFractureSubsystem::SpawnFractureActor(const FFractureRecord& Record, bool bBroadcastOpened)
{
    UWorld* World = GetWorld();
    if (!World || !Record.UniqueID.IsValid())
    {
        return nullptr;
    }

    if (const TWeakObjectPtr<AActor>* Existing = SpawnedFractures.Find(Record.UniqueID))
    {
        if (Existing->IsValid())
        {
            return Existing->Get();
        }
    }

    UClass* FractureClass = LoadClass<AActor>(nullptr, FractureSubsystemConstants::FractureClassPath);
    if (!FractureClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("FractureSubsystem: BP_Fracture introuvable à %s."), FractureSubsystemConstants::FractureClassPath);
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* FractureActor = World->SpawnActor<AActor>(FractureClass, Record.Location, Record.Rotation, SpawnParameters);
    if (!FractureActor)
    {
        return nullptr;
    }

    if (FStructProperty* FractureIDProperty = CastField<FStructProperty>(FractureActor->GetClass()->FindPropertyByName(FractureSubsystemConstants::FractureIDProperty)))
    {
        if (FGuid* FractureID = FractureIDProperty->ContainerPtrToValuePtr<FGuid>(FractureActor))
        {
            *FractureID = Record.UniqueID;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FractureSubsystem: BP_Fracture ne possède pas la propriété FractureID."));
    }

    const float Scale = ResolveFractureScale(Record.DeathContext);
    FractureActor->SetActorScale3D(FVector(Scale));
    SpawnedFractures.Add(Record.UniqueID, FractureActor);

    if (bBroadcastOpened)
    {
        if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
        {
            FGameEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.Fracture.Opened")), false);
            Event.Location = Record.Location;
            Event.Magnitude = Scale;
            Event.Payload.Add(FName(TEXT("DeathContext")), static_cast<float>(Record.DeathContext));
            EventBus->BroadcastEvent(Event);
        }
    }

    return FractureActor;
}

void UFractureSubsystem::DestroySpawnedFractures()
{
    for (const TPair<FGuid, TWeakObjectPtr<AActor>>& Pair : SpawnedFractures)
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->Destroy();
        }
    }
    SpawnedFractures.Empty();
}

FName UFractureSubsystem::GetCurrentLevelName() const
{
    const UWorld* World = GetWorld();
    return World ? FName(*World->GetMapName()) : NAME_None;
}

float UFractureSubsystem::ResolveFractureScale(int32 DeathContext) const
{
    UClass* BalanceCurveLibrary = LoadClass<UObject>(nullptr, FractureSubsystemConstants::BalanceCurveLibraryPath);
    if (!BalanceCurveLibrary)
    {
        UE_LOG(LogTemp, Warning, TEXT("FractureSubsystem: BPFL_BalanceCurves introuvable; échelle par défaut 1.0."));
        return 1.0f;
    }

    UFunction* Function = BalanceCurveLibrary->FindFunctionByName(FractureSubsystemConstants::GetFractureScaleFunction);
    UObject* DefaultObject = BalanceCurveLibrary->GetDefaultObject();
    if (!Function || !DefaultObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("FractureSubsystem: GetFractureScale introuvable; échelle par défaut 1.0."));
        return 1.0f;
    }

    struct FGetFractureScaleParams
    {
        int32 DeathContext = 0;
        UObject* WorldContext = nullptr;
        float ReturnValue = 1.0f;
    } Parameters;

    Parameters.DeathContext = DeathContext;
    Parameters.WorldContext = GetGameInstance();
    DefaultObject->ProcessEvent(Function, &Parameters);
    return Parameters.ReturnValue > 0.0f ? Parameters.ReturnValue : 1.0f;
}
